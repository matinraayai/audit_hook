/**
 * @file audit_core.cpp
 * @brief LD_AUDIT backend engine for the C++20 Audit Hooks API.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "audit_hook.hpp"
#include <link.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <shared_mutex>

// -----------------------------------------------------------------------------
// State Management & Dynamic Data Structures
// -----------------------------------------------------------------------------

struct internal_hook_t {
    std::string tool_name;
    std::string symbol_name;
    void* trampoline_ptr;
    void** original_out_ptr;
    ah_filter_mode_t filter_mode;
    std::vector<std::string> filter_libs;
};

// "Construct on First Use" idiom (Magic Statics).
// This guarantees thread-safe, dynamic allocation of the STL containers 
// exactly when they are first needed, completely avoiding the C++ static 
// initialization order fiasco, while allowing unbounded capacity.
static std::unordered_map<std::string, internal_hook_t>* get_hooks() {
    static auto* hooks = new std::unordered_map<std::string, internal_hook_t>();
    return hooks;
}

static std::shared_mutex* get_hooks_mutex() {
    static auto* mutex = new std::shared_mutex();
    return mutex;
}

static std::unordered_map<uintptr_t, std::string>* get_objects() {
    static auto* objs = new std::unordered_map<uintptr_t, std::string>();
    return objs;
}

static std::shared_mutex* get_objects_mutex() {
    static auto* mutex = new std::shared_mutex();
    return mutex;
}

// Thread-local flags for high-performance re-entrancy protection
static thread_local bool tls_hooks_paused = false;
static thread_local bool tls_hooks_ignored = false;

// Global pointer to the real dlsym, captured dynamically
static void* (*real_dlsym)(void*, const char*) = nullptr;

// -----------------------------------------------------------------------------
// Core API Exposed to C/C++ Frontend (audit_hook.hpp)
// -----------------------------------------------------------------------------

extern "C" {

int ah_register_hook(const char* tool_name, const char* symbol_name, void* hook_func, void** original_out) {
    if (!tool_name || !symbol_name || !hook_func) return -1;

    std::unique_lock lock(*get_hooks_mutex());
    (*get_hooks())[symbol_name] = {
        .tool_name = tool_name,
        .symbol_name = symbol_name,
        .trampoline_ptr = hook_func,
        .original_out_ptr = original_out,
        .filter_mode = AH_FILTER_GLOBAL,
        .filter_libs = {}
    };
    return 0;
}

int ah_set_caller_filter(const char* tool_name, ah_filter_mode_t mode, const char** libs, size_t num_libs) {
    if (!tool_name || !libs) return -1;
    
    std::unique_lock lock(*get_hooks_mutex());
    
    // Iterate through hooks and apply the filter to ones matching this tool
    for (auto& pair : *get_hooks()) {
        if (pair.second.tool_name == tool_name) {
            pair.second.filter_mode = mode;
            pair.second.filter_libs.clear();
            for (size_t i = 0; i < num_libs; ++i) {
                if (libs[i]) pair.second.filter_libs.push_back(libs[i]);
            }
        }
    }
    return 0;
}

void ah_thread_pause_hooks(void) { tls_hooks_paused = true; }
void ah_thread_resume_hooks(void) { tls_hooks_paused = false; }
void ah_thread_ignore_hooks(void) { tls_hooks_ignored = true; }

bool ah_are_hooks_paused(void) {
    return tls_hooks_paused || tls_hooks_ignored;
}

} // extern "C"

// -----------------------------------------------------------------------------
// Internal Interceptors
// -----------------------------------------------------------------------------

extern "C" void* audit_dlsym_wrapper(void* handle, const char* symbol) {
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
            if (it->second.original_out_ptr != nullptr && *(it->second.original_out_ptr) == nullptr) {
                // Pause hooks while we call real dlsym. This prevents la_symbind 
                // from intercepting this internal lookup and returning a trampoline.
                tls_hooks_paused = true;
                *(it->second.original_out_ptr) = real_dlsym(handle, symbol);
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
    if (version == 0) return 0;
    // Do not call dlopen here! The linker's global scope array 
    // is not yet ready to be resized. Wait for la_preinit.
    return LAV_CURRENT;
}

/**
 * @brief Invoked after all objects are loaded, right before the app runs.
 * This is the ONLY safe place to dlopen our plugins!
 */
void la_preinit(uintptr_t *cookie) {
    const char* plugins_env = getenv("AH_PLUGINS");
    if (plugins_env) {
        // Duplicate the environment string so we can safely tokenize it
        char* env_copy = strdup(plugins_env);
        if (env_copy) {
            char* saveptr = nullptr;
            // Tokenize by colon ':'
            char* plugin_path = strtok_r(env_copy, ":", &saveptr);
            
            while (plugin_path != nullptr) {
                // Use RTLD_LOCAL instead of RTLD_GLOBAL to avoid triggering a 
                // glibc bug inside the LM_ID_NEWLM namespace during initialization.
                void* handle = dlopen(plugin_path, RTLD_NOW | RTLD_LOCAL);
                
                if (!handle) {
                    fprintf(stderr, "[AuditCore] FATAL: Failed to load plugin '%s'\n", plugin_path);
                    fprintf(stderr, "[AuditCore] Error: %s\n", dlerror());
                }
                
                // Get the next plugin path in the list
                plugin_path = strtok_r(nullptr, ":", &saveptr);
            }
            free(env_copy);
        }
    }
}

unsigned int la_objopen(struct link_map* map, Lmid_t lmid, uintptr_t* cookie) {
    if (map && map->l_name) {
        std::unique_lock lock(*get_objects_mutex());
        (*get_objects())[*cookie] = map->l_name;
    }
    return LA_FLG_BINDTO | LA_FLG_BINDFROM;
}

unsigned int la_objclose(uintptr_t* cookie) {
    std::unique_lock lock(*get_objects_mutex());
    get_objects()->erase(*cookie);
    return 0;
}

static uintptr_t process_symbind(const char* symname, uintptr_t original_addr, unsigned int* flags, uintptr_t* refcook) {
    if (strcmp(symname, "dlsym") == 0) {
        real_dlsym = reinterpret_cast<void*(*)(void*, const char*)>(original_addr);
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
        if (!it->second.filter_libs.empty() && it->second.filter_mode != AH_FILTER_GLOBAL) {
            bool found_match = false;
            std::shared_lock obj_lock(*get_objects_mutex());
            auto obj_it = get_objects()->find(*refcook); 
            
            if (obj_it != get_objects()->end()) {
                for (const auto& lib_name : it->second.filter_libs) {
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
            *(it->second.original_out_ptr) = reinterpret_cast<void*>(original_addr);
        }
        *flags = LA_SYMB_NOPLTENTER | LA_SYMB_NOPLTEXIT;
        return reinterpret_cast<uintptr_t>(it->second.trampoline_ptr);
    }
    
    return original_addr;
}

uintptr_t la_symbind64(Elf64_Sym* sym, unsigned int ndx,
                       uintptr_t* refcook, uintptr_t* defcook,
                       unsigned int* flags, const char* symname) {
    return process_symbind(symname, sym->st_value, flags, refcook);
}

uintptr_t la_symbind32(Elf32_Sym* sym, unsigned int ndx,
                       uintptr_t* refcook, uintptr_t* defcook,
                       unsigned int* flags, const char* symname) {
    return process_symbind(symname, sym->st_value, flags, refcook);
}

} // extern "C"
