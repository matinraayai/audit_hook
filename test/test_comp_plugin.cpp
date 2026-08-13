#include "audit_hook.hpp"

// Rule 1: Wrap -> Replace
int (*orig_w_r)() = nullptr;
int wrap_w_r() { return orig_w_r ? orig_w_r() + 1 : 0; }
int replace_w_r() { return 100; }

// Rule 2: Replace -> Wrap
int (*orig_r_w)() = nullptr;
int replace_r_w() { return 200; }
int wrap_r_w() { return orig_r_w ? orig_r_w() + 1 : 0; }

// Rule 3: Wrap -> Wrap
int (*orig_w_w_1)() = nullptr;
int wrap_w_w_1() { return orig_w_w_1 ? orig_w_w_1() + 1 : 0; }
int (*orig_w_w_2)() = nullptr;
int wrap_w_w_2() { return orig_w_w_2 ? orig_w_w_2() + 10 : 0; }

// Rule 4: Replace -> Replace
int replace_r_r_1() { return 400; }
int replace_r_r_2() { return 4000; }

__attribute__((constructor)) void init() {
  // 1. Wrap -> Replace (Will trigger warning)
  audit_hooks::register_wrap<wrap_w_r, &orig_w_r>("tool_wrap", "func_w_r");
  audit_hooks::register_replace<replace_w_r>("tool_replace", "func_w_r");

  // 2. Replace -> Wrap (Silent chain)
  audit_hooks::register_replace<replace_r_w>("tool_replace", "func_r_w");
  audit_hooks::register_wrap<wrap_r_w, &orig_r_w>("tool_wrap", "func_r_w");

  // 3. Wrap -> Wrap (Silent chain)
  audit_hooks::register_wrap<wrap_w_w_1, &orig_w_w_1>("tool_wrap1", "func_w_w");
  audit_hooks::register_wrap<wrap_w_w_2, &orig_w_w_2>("tool_wrap2", "func_w_w");

  // 4. Replace -> Replace (Will trigger warning)
  audit_hooks::register_replace<replace_r_r_1>("tool_replace1", "func_r_r");
  audit_hooks::register_replace<replace_r_r_2>("tool_replace2", "func_r_r");
}