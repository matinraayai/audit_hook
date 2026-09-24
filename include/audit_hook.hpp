/**
 * @file audit_hook.hpp
 * @brief High-performance C++20 API for wrapping and replacing C/C++ functions natively via LD_AUDIT.
 * 
 * Offers safe, compile-time trampoline generation and transparent thread-local 
 * state management.
 */

#pragma once

#include "audit_hook_dynamic.h"
#include <type_traits>
#include <stdint.h>

extern "C" {
/**
 * @brief Core C API for registering a function hook.
 *
 * @param tool_name The unique identifier for the plugin.
 * @param symbol_name The target symbol to intercept (e.g., "puts").
 * @param hook_func A pointer to the execution hook.
 * @param original_out A double pointer holding the native function resolution.
 * @param dispatcher A pointer to the dynamic dispatcher thunk.
 * @param force_dynamic If true, forces the hook into Dynamic Dispatch mode bypassing static chaining.
 * @return 0 on success, or a non-zero error code if registration fails.
 */
int ah_register_hook(const char *tool_name, const char *symbol_name,
                     void *hook_func, void **original_out, void *dispatcher,
                     bool force_dynamic);

/**
 * @brief Pauses execution of audit_hook trampolines for the current thread.
 * Useful to prevent recursion during custom hook logic.
 */
void ah_thread_pause_hooks(void);

/**
 * @brief Resumes execution of audit_hook trampolines for the current thread.
 */
void ah_thread_resume_hooks(void);

/**
 * @brief Completely ignores all hooks for the current thread indefinitely.
 */
void ah_thread_ignore_hooks(void);

/**
 * @brief Checks if hooks are currently paused or ignored on the active thread.
 * @return true if hooks are bypassed, false otherwise.
 */
bool ah_are_hooks_paused(void);

/**
 * @brief Pushes the original function pointer reference onto the TLS stack.
 * @param orig_out A double pointer to the original function.
 */
void ah_push_orig_ptr(void **orig_out);

/**
 * @brief Pops the most recent original function pointer reference from the TLS stack.
 */
void ah_pop_orig_ptr(void);

/**
 * @brief Evaluates the current execution context and routes to the next wrapper or native function.
 * @param orig_out A double pointer to the original function mapping.
 * @param return_addr The caller's return address used by dladdr to identify the caller library.
 * @return A void pointer to the next function to execute in the chain.
 */
void *ah_get_next_hop(void **orig_out, void *return_addr);

/**
 * @brief Optional callback implemented by the plugin to receive dynamic load events.
 * @param libname The name of the library being loaded.
 * @param cookie The unique cookie associated with the loaded object.
 */
void ah_plugin_on_objopen(const char *libname, uintptr_t cookie);
}

/**
 * @namespace audit_hooks
 * @brief Modern C++ templated API for securely registering audit hooks with full type validation.
 */
namespace audit_hooks {

/**
 * @cond HIDDEN_SYMBOLS
 */
namespace detail {
template <typename Signature, auto HookFunc, auto OriginalPtr>
struct HookGenerator;

template <typename Ret, typename... Args, auto HookFunc, auto OriginalPtr>
struct HookGenerator<Ret (*)(Args...), HookFunc, OriginalPtr> {
  static Ret Dispatcher(Args... args) {
    void *next = ah_get_next_hop(reinterpret_cast<void **>(OriginalPtr),
                                 __builtin_return_address(0));
    return reinterpret_cast<Ret (*)(Args...)>(next)(args...);
  }

  static Ret Trampoline(Args... args) {
    ah_push_orig_ptr(reinterpret_cast<void **>(OriginalPtr));
    bool was_paused = ah_are_hooks_paused();
    if (!was_paused)
      ah_thread_pause_hooks();

    if constexpr (std::is_void_v<Ret>) {
      HookFunc(args...);
      if (!was_paused)
        ah_thread_resume_hooks();
      ah_pop_orig_ptr();
    } else {
      Ret ret = HookFunc(args...);
      if (!was_paused)
        ah_thread_resume_hooks();
      ah_pop_orig_ptr();
      return ret;
    }
  }
};
} // namespace detail
/** @endcond */

/**
 * @brief Registers a complete replacement for a function.
 * 
 * Used for zero-overhead function swapping where the original behavior 
 * is discarded.
 *
 * @tparam HookFunc A function pointer holding the replacement logic.
 * @param tool_name The unique identifier for the plugin.
 * @param symbol_name The target symbol to replace (e.g., "puts").
 * @return 0 on success, or a non-zero error code if registration fails.
 */
template <auto HookFunc>
inline int register_replace(const char *tool_name, const char *symbol_name) {
  return ah_register_hook(tool_name, symbol_name,
                          reinterpret_cast<void *>(HookFunc), nullptr, nullptr,
                          false);
}

/**
 * @brief Registers a wrapper for a dynamically linked function.
 *
 * Intercepts a function execution, runs custom logic, and manages the execution 
 * chain safely via thread-local state tracking.
 *
 * @tparam HookFunc A function pointer holding the wrapping logic.
 * @tparam OriginalPtr A double pointer where the underlying native function address will be stored.
 * @param tool_name The unique identifier for the plugin.
 * @param symbol_name The target symbol to wrap (e.g., "puts").
 * @return 0 on success, or a non-zero error code if registration fails.
 */
template <auto HookFunc, auto OriginalPtr>
inline int register_wrap(const char *tool_name, const char *symbol_name) {
  using FuncType = decltype(HookFunc);
  void *trampoline = reinterpret_cast<void *>(
      &detail::HookGenerator<FuncType, HookFunc, OriginalPtr>::Trampoline);
  void *dispatcher = reinterpret_cast<void *>(
      &detail::HookGenerator<FuncType, HookFunc, OriginalPtr>::Dispatcher);
  void **orig_out = reinterpret_cast<void **>(OriginalPtr);

  return ah_register_hook(tool_name, symbol_name, trampoline, orig_out,
                          dispatcher, false);
}

/**
 * @brief Registers a dynamic wrapper with built-in runtime toggling capabilities.
 * 
 * Enforces Dynamic Dispatch mode natively, forcing all lookups to route via `ah_get_next_hop`.
 *
 * @tparam HookFunc A function pointer holding the wrapping logic.
 * @tparam OriginalPtr A double pointer where the underlying native function address will be stored.
 * @param tool_name The unique identifier for the plugin.
 * @param symbol_name The target symbol to dynamically wrap.
 * @return 0 on success, or a non-zero error code if registration fails.
 */
template <auto HookFunc, auto OriginalPtr>
inline int register_dynamic(const char *tool_name, const char *symbol_name) {
  using FuncType = decltype(HookFunc);
  void *trampoline = reinterpret_cast<void *>(
      &detail::HookGenerator<FuncType, HookFunc, OriginalPtr>::Trampoline);
  void *dispatcher = reinterpret_cast<void *>(
      &detail::HookGenerator<FuncType, HookFunc, OriginalPtr>::Dispatcher);
  void **orig_out = reinterpret_cast<void **>(OriginalPtr);

  return ah_register_hook(tool_name, symbol_name, trampoline, orig_out,
                          dispatcher, true);
}

} // namespace audit_hooks