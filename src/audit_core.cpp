/**
 * @file audit_core.cpp
 * @brief LD_AUDIT backend engine for the C++20 Audit Hooks API.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "audit_hook.hpp"
#include <algorithm> // Required for std::find
#include <dlfcn.h>
#include <link.h>
#include <mutex>
#include <shared_mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unordered_map>
#include <vector>

// -----------------------------------------------------------------------------
// State Management & Dynamic Data Structures
// -----------------------------------------------------------------------------

struct internal_hook_t {
  std::string tool_name;
  std::string symbol_name;
  void *trampoline_ptr;
  void **original_out_ptr;
  ah_filter_mode_t filter_mode;
  std::vector<std::string> filter_libs;
  // Wrapper trampolines from outermost to innermost. Empty if and only if the
  // active hook is a replace (replaced-only). When non-empty, front() is the
  // same pointer as trampoline_ptr.
  std::vector<void *> wrapper_chain;
  // The callable beneath the wrappers: the real OS function for a pure wrap
  // chain (resolved lazily at first bind), or the wrapped replace trampoline
  // for a replace->wrap chain. nullptr for replaced-only hooks or before the
  // first lazy resolution.
  void *underlying;
};

// "Construct on First Use" idiom (Magic Statics).
// This guarantees thread-safe, dynamic allocation of the STL containers
// exactly when they are first needed, completely avoiding the C++ static
// initialization order fiasco, while allowing unbounded capacity.
static std::unordered_map<std::string, internal_hook_t> *get_hooks() {
  static auto *hooks = new std::unordered_map<std::string, internal_hook_t>();
  return hooks;
}

static std::shared_mutex *get_hooks_mutex() {
  static auto *mutex = new std::shared_mutex();
  return mutex;
}

static std::unordered_map<uintptr_t, std::string> *get_objects() {
  static auto *objs = new std::unordered_map<uintptr_t, std::string>();
  return objs;
}

static std::shared_mutex *get_objects_mutex() {
  static auto *mutex = new std::shared_mutex();
  return mutex;
}

// Thread-local flags for high-performance re-entrancy protection
static thread_local bool tls_hooks_paused = false;
static thread_local bool tls_hooks_ignored = false;

// Global pointer to the real dlsym, captured dynamically
static void *(*real_dlsym)(void *, const char *) = nullptr;

// -----------------------------------------------------------------------------
// Core API Exposed to C/C++ Frontend (audit_hook.hpp)
// -----------------------------------------------------------------------------

extern "C" {

int ah_register_hook(const char *tool_name, const char *symbol_name,
                     void *hook_func, void **original_out) {
  if (!tool_name || !symbol_name || !hook_func)
    return -1;

  std::unique_lock lock(*get_hooks_mutex());
  auto &hooks = *get_hooks();
  auto it = hooks.find(symbol_name);

  if (it != hooks.end()) {
    bool is_new_replace = (original_out == nullptr);
    bool is_old_replace = (it->second.original_out_ptr == nullptr);

    if (is_new_replace && !is_old_replace) {
      // RULE 1: Wrap followed by Replace
      // The wrapping is discarded and a warning is emitted.
      fprintf(stderr,
              "[AuditCore] WARNING: Symbol '%s' was wrapped by '%s', but is "
              "now being entirely replaced by '%s'. The previous wrapping is "
              "discarded.\n",
              symbol_name, it->second.tool_name.c_str(), tool_name);

      it->second.tool_name = tool_name;
      it->second.trampoline_ptr = hook_func;
      it->second.original_out_ptr = nullptr;
      it->second.wrapper_chain.clear();
      it->second.underlying = nullptr;

      // Reset filters for the new replacement
      it->second.filter_mode = AH_FILTER_GLOBAL;
      it->second.filter_libs.clear();
    } else if (!is_new_replace) {
      // RULES 2 & 3: Replace followed by Wrap, OR Wrap followed by Wrap

      // Chain them: The new wrapper's "original" pointer gets the address
      // of the currently active trampoline.
      *original_out = it->second.trampoline_ptr;

      // Record the chain from outermost to innermost so it can be traversed.
      // Wrapping a replace: the replace becomes the underlying callable.
      // Wrapping a wrapper: prepend the new wrapper to the recorded chain.
      if (it->second.wrapper_chain.empty()) {
        it->second.underlying = it->second.trampoline_ptr;
        it->second.wrapper_chain.push_back(hook_func);
      } else {
        it->second.wrapper_chain.insert(it->second.wrapper_chain.begin(),
                                        hook_func);
      }

      // Update the active trampoline to the new wrapper
      it->second.tool_name = tool_name;
      it->second.trampoline_ptr = hook_func;
    } else {
      // RULE 4: Replace followed by Replace
      // Overwrite the old replace with the new one and emit a warning.
      fprintf(stderr,
              "[AuditCore] WARNING: Symbol '%s' was already replaced by '%s', "
              "but is now being replaced again by '%s'. The previous "
              "replacement is discarded.\n",
              symbol_name, it->second.tool_name.c_str(), tool_name);

      it->second.tool_name = tool_name;
      it->second.trampoline_ptr = hook_func;
      it->second.original_out_ptr = nullptr;
      it->second.wrapper_chain.clear();
      it->second.underlying = nullptr;

      // Reset filters for the new replacement
      it->second.filter_mode = AH_FILTER_GLOBAL;
      it->second.filter_libs.clear();
    }
  } else {
    // First time this symbol is hooked
    hooks[symbol_name] = {.tool_name = tool_name,
                          .symbol_name = symbol_name,
                          .trampoline_ptr = hook_func,
                          .original_out_ptr = original_out,
                          .filter_mode = AH_FILTER_GLOBAL,
                          .filter_libs = {},
                          .wrapper_chain = original_out
                                               ? std::vector<void *>{hook_func}
                                               : std::vector<void *>{},
                          .underlying = nullptr};
  }
  return 0;
}

int ah_set_caller_filter(const char *tool_name, ah_filter_mode_t mode,
                         const char **libs, size_t num_libs) {
  if (!tool_name || !libs)
    return -1;

  std::unique_lock lock(*get_hooks_mutex());

  // Iterate through hooks and apply the filter to ones matching this tool
  for (auto &pair : *get_hooks()) {
    if (pair.second.tool_name == tool_name) {
      auto &hook = pair.second;

      if (mode == AH_FILTER_INCLUDE) {
        if (hook.filter_mode == AH_FILTER_EXCLUDE) {
          // Rule: Exclude followed by Include (Inverted Set Difference)
          // We remove the newly "included" libraries from the existing
          // "exclude" list.
          for (size_t i = 0; i < num_libs; ++i) {
            if (libs[i]) {
              std::string target = libs[i];
              auto it = std::find(hook.filter_libs.begin(),
                                  hook.filter_libs.end(), target);

              if (it != hook.filter_libs.end()) {
                hook.filter_libs.erase(it);
              } else {
                // Symmetrical warning: attempting to include a library not
                // currently excluded
                fprintf(stderr,
                        "[AuditCore] WARNING: Tool '%s' attempted to include "
                        "library '%s' on symbol '%s', but it was not in the "
                        "active EXCLUDE list.\n",
                        tool_name, target.c_str(), hook.symbol_name.c_str());
              }
            }
          }
        } else {
          if (hook.filter_mode == AH_FILTER_GLOBAL) {
            // Rule: Global followed by Include
            hook.filter_mode = AH_FILTER_INCLUDE;
            hook.filter_libs.clear();
          }

          // Rule: Append new libraries, silently ignoring duplicates
          for (size_t i = 0; i < num_libs; ++i) {
            if (libs[i]) {
              std::string new_lib = libs[i];
              if (std::find(hook.filter_libs.begin(), hook.filter_libs.end(),
                            new_lib) == hook.filter_libs.end()) {
                hook.filter_libs.push_back(new_lib);
              }
            }
          }
        }
      } else if (mode == AH_FILTER_EXCLUDE) {
        if (hook.filter_mode == AH_FILTER_GLOBAL ||
            hook.filter_mode == AH_FILTER_EXCLUDE) {
          // Rule: Global followed by Exclude, OR Exclude followed by Exclude
          hook.filter_mode = AH_FILTER_EXCLUDE;

          // Append new exclusions, silently ignoring duplicates
          for (size_t i = 0; i < num_libs; ++i) {
            if (libs[i]) {
              std::string new_lib = libs[i];
              if (std::find(hook.filter_libs.begin(), hook.filter_libs.end(),
                            new_lib) == hook.filter_libs.end()) {
                hook.filter_libs.push_back(new_lib);
              }
            }
          }
        } else if (hook.filter_mode == AH_FILTER_INCLUDE) {
          // Rule: Include followed by Exclude (Set Difference)
          for (size_t i = 0; i < num_libs; ++i) {
            if (libs[i]) {
              std::string target = libs[i];
              auto it = std::find(hook.filter_libs.begin(),
                                  hook.filter_libs.end(), target);

              if (it != hook.filter_libs.end()) {
                hook.filter_libs.erase(it);
              } else {
                // Rule: Warn if excluded library is not in the active Include
                // list
                fprintf(stderr,
                        "[AuditCore] WARNING: Tool '%s' attempted to exclude "
                        "library '%s' on symbol '%s', but it was not in the "
                        "active INCLUDE list.\n",
                        tool_name, target.c_str(), hook.symbol_name.c_str());
              }
            }
          }
        }
      }
    }
  }
  return 0;
}

void ah_thread_pause_hooks(void) { tls_hooks_paused = true; }
void ah_thread_resume_hooks(void) { tls_hooks_paused = false; }
void ah_thread_ignore_hooks(void) { tls_hooks_ignored = true; }

bool ah_are_hooks_paused(void) { return tls_hooks_paused || tls_hooks_ignored; }

} // extern "C"

// -----------------------------------------------------------------------------
// Internal Interceptors
// -----------------------------------------------------------------------------

extern "C" void *audit_dlsym_wrapper(void *handle, const char *symbol) {
  // If hooks are already paused, bypass interception completely and
  // return the REAL address, not the trampoline.
  if (tls_hooks_paused || tls_hooks_ignored) {
    return real_dlsym ? real_dlsym(handle, symbol) : nullptr;
  }

  // 1. Check if the requested symbol is hooked
  {
    std::shared_lock lock(*get_hooks_mutex());
    auto it = get_hooks()->find(symbol);
    if (it != get_hooks()->end()) {

      // If we haven't populated the original pointer for the wrapper yet,
      // we must do it now using the real dlsym.
      if (it->second.original_out_ptr != nullptr &&
          *(it->second.original_out_ptr) == nullptr) {
        // Pause hooks while we call real dlsym. This prevents la_symbind
        // from intercepting this internal lookup and returning a trampoline.
        tls_hooks_paused = true;
        void *real_func = real_dlsym(handle, symbol);
        *(it->second.original_out_ptr) = real_func;
        it->second.underlying = real_func;
        tls_hooks_paused = false;
      }

      // Return the C++ trampoline
      return it->second.trampoline_ptr;
    }
  }

  // 2. Not hooked, pass through to real dlsym
  if (real_dlsym) {
    return real_dlsym(handle, symbol);
  }

  return nullptr;
}

// -----------------------------------------------------------------------------
// LD_AUDIT Linker Callbacks
// -----------------------------------------------------------------------------

extern "C" {

/**
 * @brief Handshake with ld.so.
 */
unsigned int la_version(unsigned int version) {
  if (version == 0)
    return 0;
  // Do not call dlopen here! The linker's global scope array
  // is not yet ready to be resized. Wait for la_preinit.
  return LAV_CURRENT;
}

/**
 * @brief Invoked after all objects are loaded, right before the app runs.
 * This is the ONLY safe place to dlopen our plugins!
 */
void la_preinit(uintptr_t *cookie) {
  const char *plugins_env = getenv("AH_PLUGINS");
  if (plugins_env) {
    // Duplicate the environment string so we can safely tokenize it
    char *env_copy = strdup(plugins_env);
    if (env_copy) {
      char *saveptr = nullptr;
      // Tokenize by colon ':'
      char *plugin_path = strtok_r(env_copy, ":", &saveptr);

      while (plugin_path != nullptr) {
        // Use RTLD_LOCAL instead of RTLD_GLOBAL to avoid triggering a
        // glibc bug inside the LM_ID_NEWLM namespace during initialization.
        void *handle = dlopen(plugin_path, RTLD_NOW | RTLD_LOCAL);

        if (!handle) {
          fprintf(stderr, "[AuditCore] FATAL: Failed to load plugin '%s'\n",
                  plugin_path);
          fprintf(stderr, "[AuditCore] Error: %s\n", dlerror());
        }

        // Get the next plugin path in the list
        plugin_path = strtok_r(nullptr, ":", &saveptr);
      }
      free(env_copy);
    }
  }
}

unsigned int la_objopen(struct link_map *map, Lmid_t lmid, uintptr_t *cookie) {
  if (map && map->l_name) {
    std::unique_lock lock(*get_objects_mutex());
    (*get_objects())[*cookie] = map->l_name;
  }
  return LA_FLG_BINDTO | LA_FLG_BINDFROM;
}

unsigned int la_objclose(uintptr_t *cookie) {
  std::unique_lock lock(*get_objects_mutex());
  get_objects()->erase(*cookie);
  return 0;
}

static uintptr_t process_symbind(const char *symname, uintptr_t original_addr,
                                 unsigned int *flags, uintptr_t *refcook) {
  if (strcmp(symname, "dlsym") == 0) {
    real_dlsym =
        reinterpret_cast<void *(*)(void *, const char *)>(original_addr);
    *flags = LA_SYMB_NOPLTENTER | LA_SYMB_NOPLTEXIT;
    return reinterpret_cast<uintptr_t>(&audit_dlsym_wrapper);
  }

  // If hooks are paused, bypass completely. This stops dlsym's internal
  // la_symbind trigger from replacing our internal lookups with a trampoline.
  if (tls_hooks_paused || tls_hooks_ignored) {
    return original_addr;
  }

  std::shared_lock lock(*get_hooks_mutex());
  auto it = get_hooks()->find(symname);
  if (it != get_hooks()->end()) {

    // Check the caller filter if libraries are specified and it's not global
    if (!it->second.filter_libs.empty() &&
        it->second.filter_mode != AH_FILTER_GLOBAL) {
      bool found_match = false;
      std::shared_lock obj_lock(*get_objects_mutex());
      auto obj_it = get_objects()->find(*refcook);

      if (obj_it != get_objects()->end()) {
        for (const auto &lib_name : it->second.filter_libs) {
          if (obj_it->second.find(lib_name) != std::string::npos) {
            found_match = true;
            break;
          }
        }
      }

      // Bypass the hook if the logic dictates it
      if (it->second.filter_mode == AH_FILTER_INCLUDE && !found_match) {
        return original_addr; // Not in include list, bypass hook
      } else if (it->second.filter_mode == AH_FILTER_EXCLUDE && found_match) {
        return original_addr; // In exclude list, bypass hook
      }
    }

    if (it->second.original_out_ptr) {
      *(it->second.original_out_ptr) = reinterpret_cast<void *>(original_addr);
      it->second.underlying = reinterpret_cast<void *>(original_addr);
    }
    *flags = LA_SYMB_NOPLTENTER | LA_SYMB_NOPLTEXIT;
    return reinterpret_cast<uintptr_t>(it->second.trampoline_ptr);
  }

  return original_addr;
}

uintptr_t la_symbind64(Elf64_Sym *sym, unsigned int ndx, uintptr_t *refcook,
                       uintptr_t *defcook, unsigned int *flags,
                       const char *symname) {
  return process_symbind(symname, sym->st_value, flags, refcook);
}

uintptr_t la_symbind32(Elf32_Sym *sym, unsigned int ndx, uintptr_t *refcook,
                       uintptr_t *defcook, unsigned int *flags,
                       const char *symname) {
  return process_symbind(symname, sym->st_value, flags, refcook);
}

} // extern "C"
