#include "audit_hook.hpp"
#include <stdio.h>

// Entirely replaces puts
int my_puts(const char *str) {
  // Use fputs to avoid GCC optimizing printf("...\n") into puts() and
  // recursing!
  fputs("SUCCESS: Function was Replaced!\n", stdout);
  return 0;
}

__attribute__((constructor)) void init() {
  audit_hooks::register_replace<my_puts>("test_replace", "puts");
}