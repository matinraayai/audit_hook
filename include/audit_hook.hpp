/**
 * @file audit_hook.hpp
 * @brief Public API for the C++20 Audit Hooks framework.
 *
 * This header provides both the raw C ABI boundary for communicating with the
 * LD_AUDIT backend engine, as well as a modern, zero-overhead C++20 frontend
 * for registering compile-time trampolines and configuring caller filters.
 */

#pragma once

#include <concepts>
#include <cstddef>
#include <ranges>
#include <vector>

// -----------------------------------------------------------------------------
// Core C ABI & Types
// -----------------------------------------------------------------------------

/**
 * @brief Defines the filtering behavior for a registered hook.
 */
typedef enum {
  AH_FILTER_GLOBAL = 0, /*!< Hook applies to all callers (Default) */
  AH_FILTER_INCLUDE, /*!< Hook applies ONLY if the caller is in the filter list
                      */
  AH_FILTER_EXCLUDE /*!< Hook applies UNLESS the caller is in the filter list */
} ah_filter_mode_t;

extern "C" {
/**
 * @brief Raw ABI for registering a hook with the LD_AUDIT backend.
 * @param tool_name The name of the tool registering the hook.
 * @param symbol_name The symbol to intercept.
 * @param hook_func Pointer to the trampoline/wrapper function.
 * @param original_out Pointer to where the original function address should be
 * stored.
 * @return 0 on success, -1 on failure.
 */
int ah_register_hook(const char *tool_name, const char *symbol_name,
                     void *hook_func, void **original_out);

/**
 * @brief Raw ABI for setting a caller-based filter on a specific hook.
 * @param tool_name The name of the tool that owns the hook.
 * @param mode The filtering mode (Global, Include, Exclude).
 * @param libs Array of C-strings representing library names to filter.
 * @param num_libs The number of libraries in the array.
 * @return 0 on success, -1 on failure.
 */
int ah_set_caller_filter(const char *tool_name, ah_filter_mode_t mode,
                         const char **libs, size_t num_libs);

/**
 * @brief Manually pause hook interception for the current thread.
 */
void ah_thread_pause_hooks(void);

/**
 * @brief Manually resume hook interception for the current thread.
 */
void ah_thread_resume_hooks(void);

/**
 * @brief Permanently ignore hook interception for the current thread.
 */
void ah_thread_ignore_hooks(void);

/**
 * @brief Check if hooks are currently paused or ignored for the current thread.
 * @return true if hooks are bypassed, false otherwise.
 */
bool ah_are_hooks_paused(void);
}

// -----------------------------------------------------------------------------
// Modern C++20 Frontend API
// -----------------------------------------------------------------------------

/**
 * @brief Sets a caller filter for a registered hook using any iterable C++20
 * range.
 *
 * This concept-constrained template overload allows you to pass any iterable
 * collection (e.g., std::vector, std::array, std::span) of strings or char
 * pointers. It automatically translates the range into the contiguous C-array
 * required by the ABI.
 *
 * @tparam Range Any std::ranges::forward_range whose elements convert to const
 * char*.
 * @param tool_name The name of the tool that owns the hook.
 * @param mode The filtering mode (AH_FILTER_INCLUDE or AH_FILTER_EXCLUDE).
 * @param libs The iterable range of library names.
 * @return 0 on success, -1 on failure.
 */
template <std::ranges::forward_range Range>
  requires std::convertible_to<std::ranges::range_value_t<Range>, const char *>
inline int ah_set_caller_filter(const char *tool_name, ah_filter_mode_t mode,
                                const Range &libs) {
  std::vector<const char *> contiguous_libs;

  // Optimization: If the range knows its size at compile/runtime, pre-allocate
  if constexpr (std::ranges::sized_range<Range>) {
    contiguous_libs.reserve(std::ranges::size(libs));
  }

  for (const auto &lib : libs) {
    contiguous_libs.push_back(lib);
  }

  // Route down to the C ABI
  return ah_set_caller_filter(tool_name, mode, contiguous_libs.data(),
                              contiguous_libs.size());
}

namespace audit_hooks {

namespace detail {
/**
 * @brief Internal trampoline generator to manage re-entrancy automatically.
 */
template <typename Signature, auto HookFunc, auto OriginalPtr>
struct HookGenerator;

template <typename Ret, typename... Args, auto HookFunc, auto OriginalPtr>
struct HookGenerator<Ret (*)(Args...), HookFunc, OriginalPtr> {
  static Ret Trampoline(Args... args) {
    // Auto-pause hooks to prevent re-entrancy loops when the wrapper executes
    bool was_paused = ah_are_hooks_paused();
    if (!was_paused)
      ah_thread_pause_hooks();

    if constexpr (std::is_void_v<Ret>) {
      HookFunc(args...);
      if (!was_paused)
        ah_thread_resume_hooks();
    } else {
      Ret ret = HookFunc(args...);
      if (!was_paused)
        ah_thread_resume_hooks();
      return ret;
    }
  }
};
} // namespace detail

/**
 * @brief Replaces a function entirely (zero overhead).
 *
 * Use this when you want to completely overwrite a target function and do not
 * need to call the original underlying logic. This bypasses the auto-pausing
 * trampoline completely for maximum performance.
 *
 * @tparam HookFunc The C++ replacement function.
 * @param tool_name The namespace/name of your tool.
 * @param symbol_name The exact ELF symbol name to replace.
 * @return 0 on success, -1 on failure.
 */
template <auto HookFunc>
inline int register_replace(const char *tool_name, const char *symbol_name) {
  // For pure replacement, we bypass the auto-pausing trampoline completely.
  return ah_register_hook(tool_name, symbol_name,
                          reinterpret_cast<void *>(HookFunc), nullptr);
}

/**
 * @brief Wraps a function, automatically managing re-entrancy.
 *
 * Use this when you want to intercept a target function, execute your own
 * logic, and optionally call the original underlying function pointer. This
 * automatically handles pausing thread-local hooks to prevent infinite
 * recursion.
 *
 * @tparam HookFunc The C++ wrapper function.
 * @tparam OriginalPtr A pointer to the variable where the real function address
 * will be stored.
 * @param tool_name The namespace/name of your tool.
 * @param symbol_name The exact ELF symbol name to wrap.
 * @return 0 on success, -1 on failure.
 */
template <auto HookFunc, auto OriginalPtr>
inline int register_wrap(const char *tool_name, const char *symbol_name) {
  using FuncType = decltype(HookFunc);
  void *trampoline = reinterpret_cast<void *>(
      &detail::HookGenerator<FuncType, HookFunc, OriginalPtr>::Trampoline);
  void **orig_out = reinterpret_cast<void **>(OriginalPtr);

  return ah_register_hook(tool_name, symbol_name, trampoline, orig_out);
}

} // namespace audit_hooks