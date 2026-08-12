#include "audit_hook.hpp"
#include <stdio.h>

int (*real_puts)(const char*) = nullptr;

// Wraps puts, calling the original
int my_puts(const char* str) {
    printf("SUCCESS: Function was Wrapped! -> ");
    return real_puts(str);
}

__attribute__((constructor)) void init() {
    audit_hooks::register_wrap<my_puts, &real_puts>("test_wrap", "puts");
}