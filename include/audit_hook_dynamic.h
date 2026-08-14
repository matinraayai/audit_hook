#ifndef AUDIT_HOOK_DYNAMIC_H
#define AUDIT_HOOK_DYNAMIC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  AH_FILTER_GLOBAL = 0,
  AH_FILTER_INCLUDE,
  AH_FILTER_EXCLUDE
} ah_filter_mode_t;

typedef enum {
  AH_SUCCESS = 0,
  AH_ERR_NOT_HOOKED = -1,
  AH_ERR_NOT_DYNAMIC = -2,
  AH_ERR_NOT_GLOBAL = -3,
  AH_ERR_MULTIPLE_RULES = -4
} ah_err_t;

int ah_set_caller_filter(const char *tool_name, ah_filter_mode_t mode,
                         const char **libs, size_t num_libs);

/**
 * @brief Dynamically updates the target trampoline for a registered hook.
 */
int ah_set_target(const char *tool_name, void *new_target);

/**
 * @brief Retrieves the current target of a global dynamic dispatch hook.
 * 
 * @return AH_SUCCESS and sets out_target if the hook is strictly global.
 *         Returns specific ah_err_t codes if the constraints are violated.
 */
ah_err_t ah_get_target(const char *tool_name, void **out_target);

#ifdef __cplusplus
} /* extern "C" */

/* --- Modern C++ Overloads --- */
#include <concepts>
#include <ranges>
#include <vector>

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

#endif /* __cplusplus */

#endif /* AUDIT_HOOK_DYNAMIC_H */