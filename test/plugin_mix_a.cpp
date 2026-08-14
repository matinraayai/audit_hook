#include "audit_hook.hpp"

// Inner Replacement (Completely overrides OS function)
int replace_A() { return 100; }

__attribute__((constructor)) void init() {
  audit_hooks::register_replace<replace_A>("plugin_mix_a", "func_three");
}