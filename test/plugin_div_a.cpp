#include "audit_hook.hpp"

int (*orig_A)() = nullptr;

int wrap_A() { return orig_A ? orig_A() + 1 : 0; }

__attribute__((constructor)) void init() {
  audit_hooks::register_wrap<wrap_A, &orig_A>("plugin_div_a", "func_one");
}