#include "audit_hook.hpp"

int correct_return_four() { return 4; }
int correct_return_five() { return 5; }
int correct_return_six() { return 6; }

__attribute__((constructor)) void init() {
    audit_hooks::register_replace<correct_return_four>("dlopen_test", "return_four");
    audit_hooks::register_replace<correct_return_five>("dlopen_test", "return_five");
    audit_hooks::register_replace<correct_return_six>("dlopen_test", "return_six");
}