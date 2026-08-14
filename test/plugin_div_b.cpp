#include "audit_hook.hpp"

int (*orig_B)() = nullptr;

int wrap_B() { return orig_B ? orig_B() + 1 : 0; }

__attribute__((constructor)) void init() {
  audit_hooks::register_wrap<wrap_B, &orig_B>("plugin_div_b", "func_one");
  
  // Triggers the Divergent Chain warning!
  const char *libs[] = {"libdispatch_x.so"};
  ah_set_caller_filter("plugin_div_b", AH_FILTER_INCLUDE, libs);
}