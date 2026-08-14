#include "audit_hook.hpp"

int (*orig_stat_B)() = nullptr;

int wrap_stat_B() { return orig_stat_B ? orig_stat_B() + 1 : 0; }

__attribute__((constructor)) void init() {
  audit_hooks::register_wrap<wrap_stat_B, &orig_stat_B>("plugin_stat_b", "func_two");
  // NO FILTER - Remains Global
}