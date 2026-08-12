/**
 * @file audit_hook.hpp
 * @brief High-performance, type-safe LD_AUDIT instrumentation API.
 * 
 * Combines a minimal C runtime interface for LD_AUDIT symbol bindings
 * with a C++20 variadic template frontend for zero-overhead, ABI-safe
 * trampoline generation.
 */

#ifndef AUDIT_HOOK_HPP
#define AUDIT_HOOK_HPP

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
#include <utility>
#include <type_traits>

extern "C" {
#endif

/**
 * @enum ah_filter_mode_t
 * @brief Defines how caller filtering is applied during la_symbind.
 */
typedef enum {
    AH_FILTER_GLOBAL = 0, /* Hook applies to all callers (LD_PRELOAD style) */
    AH_FILTER_INCLUDE,    /* Hook applies ONLY to specified caller libraries */
    AH_FILTER_EXCLUDE     /* Hook applies to all EXCEPT specified caller libraries */
} ah_filter_mode_t;

/**
 * @brief Registers a hook with the LD_AUDIT engine.
 * 
 * @param tool_name Identifier for the tool registering the hook.
 * @param symbol_name Name of the target symbol (e.g., "malloc").
 * @param hook_func Address of the wrapper/trampoline or replacement function.
 * @param original_out OUT: Pointer address where la_symbind will write 
 *                     the resolved real function pointer during dynamic binding.
 * @return 0 on success, non-zero on error.
 */
int ah_register_hook(const char* tool_name, 
                     const char* symbol_name, 
                     void* hook_func, 
                     void** original_out);

/**
 * @brief Restricts which calling libraries trigger hooks using la_symbind's refcook.
 */
int ah_set_caller_filter(const char* tool_name, 
                         ah_filter_mode_t mode, 
                         const char** object_names, 
                         size_t count);

/**
 * @brief Pauses hook execution for the current thread (TLS state flag).
 */
void ah_thread_pause_hooks(void);

/**
 * @brief Resumes hook execution for the current thread (TLS state flag).
 */
void ah_thread_resume_hooks(void);

/**
 * @brief Permanently ignores all hooks for the calling thread (useful for background workers).
 */
void ah_thread_ignore_hooks(void);

/**
 * @brief Fast inline query to check if hooks are currently paused on this thread.
 */
bool ah_are_hooks_paused(void);

#ifdef __cplusplus
} // extern "C"

namespace audit_hooks {

/**
 * @brief RAII Guard for thread-local hook pausing.
 */
class [[nodiscard]] ScopedPause {
public:
    ScopedPause() noexcept { ah_thread_pause_hooks(); }
    ~ScopedPause() noexcept { ah_thread_resume_hooks(); }

    ScopedPause(const ScopedPause&) = delete;
    ScopedPause& operator=(const ScopedPause&) = delete;
    ScopedPause(ScopedPause&&) = delete;
    ScopedPause& operator=(ScopedPause&&) = delete;
};

namespace detail {

// Helper template to generate compile-time trampolines for any C/C++ function signature.
template <typename FuncSig, auto WrapperFunc, auto OriginalFuncPtr>
struct HookGenerator;

template <typename Ret, typename... Args, auto WrapperFunc, auto OriginalFuncPtr>
struct HookGenerator<Ret(*)(Args...), WrapperFunc, OriginalFuncPtr> {

    /**
     * @brief C-compatible static trampoline instantiated per unique hook signature.
     * 
     * The C++ compiler generates the exact CPU calling convention prologue/epilogue
     * and register management for (Args...) -> Ret automatically.
     */
    static Ret Trampoline(Args... args) {
        // 1. Re-entrancy check: If this thread is paused, bypass straight to the real function.
        if (ah_are_hooks_paused()) {
            return (*OriginalFuncPtr)(std::forward<Args>(args)...);
        }

        // 2. Auto-pause hooks before entering user wrapper to prevent infinite recursion
        //    from inside the wrapper or standard library calls.
        ah_thread_pause_hooks();

        // 3. Perfect forwarding to user wrapper with void / non-void handling
        if constexpr (std::is_void_v<Ret>) {
            WrapperFunc(std::forward<Args>(args)...);
            ah_thread_resume_hooks();
        } else {
            decltype(auto) result = WrapperFunc(std::forward<Args>(args)...);
            ah_thread_resume_hooks();
            return result;
        }
    }
};

} // namespace detail

/**
 * @brief Wraps a target symbol using a C++20 compile-time generated trampoline.
 * 
 * Automatically handles argument forwarding, calling conventions, and TLS 
 * re-entrancy prevention.
 * 
 * @tparam WrapperFunc Pointer to your wrapper function.
 * @tparam OriginalFuncPtr Pointer to the global function pointer storing the real address.
 * @param tool_name Identifier for your tool.
 * @param symbol_name Symbol to intercept (e.g., "malloc", "open").
 * @return 0 on success, non-zero on failure.
 */
template <auto WrapperFunc, auto OriginalFuncPtr>
inline int register_wrap(const char* tool_name, const char* symbol_name) {
    using FuncType = std::remove_pointer_t<decltype(WrapperFunc)>;
    
    static_assert(std::is_function_v<FuncType>, 
        "WrapperFunc must be a valid function pointer.");

    using Generator = detail::HookGenerator<decltype(WrapperFunc), WrapperFunc, OriginalFuncPtr>;
    
    return ah_register_hook(
        tool_name,
        symbol_name,
        reinterpret_cast<void*>(&Generator::Trampoline),
        reinterpret_cast<void**>(OriginalFuncPtr)
    );
}

/**
 * @brief Completely replaces a target symbol without trampoline overhead or TLS state tracking.
 * 
 * @tparam ReplacementFunc Pointer to the replacement function.
 * @param tool_name Identifier for your tool.
 * @param symbol_name Symbol to intercept.
 * @param original_out Optional OUT pointer to receive the original function address.
 * @return 0 on success, non-zero on failure.
 */
template <auto ReplacementFunc>
inline int register_replace(const char* tool_name, const char* symbol_name, void** original_out = nullptr) {
    return ah_register_hook(
        tool_name,
        symbol_name,
        reinterpret_cast<void*>(ReplacementFunc),
        original_out
    );
}

} // namespace audit_hooks

#endif // __cplusplus

#endif // AUDIT_HOOK_HPP