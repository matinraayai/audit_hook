#include "audit_hook_dynamic.h"

extern "C" {

int ah_set_caller_filter(const char *tool_name, ah_filter_mode_t mode,
                         const char **libs, size_t num_libs) {
  return -1; // Stub
}

int ah_set_target(const char *tool_name, void *new_target) {
  return -1; // Stub
}

ah_err_t ah_get_target(const char *tool_name, void **out_target) {
  return AH_ERR_NOT_HOOKED; // Stub
}

} // extern "C"