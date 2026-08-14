#include "audit_hook.hpp"

int (*orig_mix_B)() = nullptr;

// Outer Wrapper
int wrap_mix_B() { return orig_mix_B ? orig_mix_B() + 1 : 0; }

__attribute__((constructor)) void init() {
  audit_hooks::register_wrap<wrap_mix_B, &orig_mix_B>("plugin_mix_b", "func_three");
  
  // Triggers Divergent Chain warning!
  const char *libs[] = {"libdispatch_x.so"};
  ah_set_caller_filter("plugin_mix_b", AH_FILTER_INCLUDE, libs);
}