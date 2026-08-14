#include "audit_hook.hpp"

int (*orig_stat_A)() = nullptr;

int wrap_stat_A() { return orig_stat_A ? orig_stat_A() + 1 : 0; }

__attribute__((constructor)) void init() {
  audit_hooks::register_wrap<wrap_stat_A, &orig_stat_A>("plugin_stat_a", "func_two");
}