#include "audit_hook.hpp"

int (*orig_real)() = nullptr;

// Hooked version adds 1
int wrap_real() { return orig_real ? orig_real() + 1 : 0; }

__attribute__((constructor)) void init() {
  // Register as a dynamic dispatcher to support runtime toggling
  audit_hooks::register_dynamic<wrap_real, &orig_real>("tool_dyn",
                                                       "func_three");

  // Start with the main application explicitly excluded
  const char *libs[] = {"app_fp_dynamic"};
  ah_set_caller_filter("tool_dyn", AH_FILTER_EXCLUDE, libs, 1);
}