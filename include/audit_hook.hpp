#pragma once

#include <concepts>
#include <cstddef>
#include <ranges>
#include <vector>

typedef enum {
  AH_FILTER_GLOBAL = 0,
  AH_FILTER_INCLUDE,
  AH_FILTER_EXCLUDE
} ah_filter_mode_t;

extern "C" {
  int ah_register_hook(const char *tool_name, const char *symbol_name,
                       void *hook_func, void **original_out, void *dispatcher);
  int ah_set_caller_filter(const char *tool_name, ah_filter_mode_t mode,
                           const char **libs, size_t num_libs);

  void ah_thread_pause_hooks(void);
  void ah_thread_resume_hooks(void);
  void ah_thread_ignore_hooks(void);
  bool ah_are_hooks_paused(void);

  // New TLS Caller Stack Management
  void ah_push_caller(void *addr);
  void ah_pop_caller(void);
  void *ah_get_next_hop(void **orig_out);
}

template <std::ranges::forward_range Range>
  requires std::convertible_to<std::ranges::range_value_t<Range>, const char *>
inline int ah_set_caller_filter(const char *tool_name, ah_filter_mode_t mode,
                                const Range &libs) {
  std::vector<const char *> contiguous_libs;

  if constexpr (std::ranges::sized_range<Range>) {
    contiguous_libs.reserve(std::ranges::size(libs));
  }

  for (const auto &lib : libs) {
    contiguous_libs.push_back(lib);
  }

  return ah_set_caller_filter(tool_name, mode, contiguous_libs.data(),
                              contiguous_libs.size());
}

namespace audit_hooks {

namespace detail {
  template <typename Signature, auto HookFunc, auto OriginalPtr>
  struct HookGenerator;

  template <typename Ret, typename... Args, auto HookFunc, auto OriginalPtr>
  struct HookGenerator<Ret (*)(Args...), HookFunc, OriginalPtr> {

    // The strongly-typed dynamic dispatcher
    static Ret Dispatcher(Args... args) {
      void *next = ah_get_next_hop(reinterpret_cast<void **>(OriginalPtr));
      return reinterpret_cast<Ret (*)(Args...)>(next)(args...);
    }

    static Ret Trampoline(Args... args) {
      // Push the caller's address to the TLS stack for the dispatcher to read
      ah_push_caller(__builtin_return_address(0));

      bool was_paused = ah_are_hooks_paused();
      if (!was_paused)
        ah_thread_pause_hooks();

      if constexpr (std::is_void_v<Ret>) {
        HookFunc(args...);
        if (!was_paused)
          ah_thread_resume_hooks();
        ah_pop_caller();
      } else {
        Ret ret = HookFunc(args...);
        if (!was_paused)
          ah_thread_resume_hooks();
        ah_pop_caller();
        return ret;
      }
    }
  };
} // namespace detail

template <auto HookFunc>
inline int register_replace(const char *tool_name, const char *symbol_name) {
  return ah_register_hook(tool_name, symbol_name,
                          reinterpret_cast<void *>(HookFunc), nullptr, nullptr);
}

template <auto HookFunc, auto OriginalPtr>
inline int register_wrap(const char *tool_name, const char *symbol_name) {
  using FuncType = decltype(HookFunc);
  void *trampoline = reinterpret_cast<void *>(
      &detail::HookGenerator<FuncType, HookFunc, OriginalPtr>::Trampoline);
  void *dispatcher = reinterpret_cast<void *>(
      &detail::HookGenerator<FuncType, HookFunc, OriginalPtr>::Dispatcher);
  void **orig_out = reinterpret_cast<void **>(OriginalPtr);

  return ah_register_hook(tool_name, symbol_name, trampoline, orig_out,
                          dispatcher);
}

} // namespace audit_hooks