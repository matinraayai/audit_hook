#include "audit_hook.hpp"
#include <stdio.h>

int (*real_target_func)(int) = nullptr;

int my_target_func(int val) {
  printf(
      "SUCCESS: Intercepted dlsym call dynamically! Modifying argument...\n");
  return real_target_func(val + 100);
}

__attribute__((constructor)) void init() {
  audit_hooks::register_wrap<my_target_func, &real_target_func>(
      "test_dlsym", "target_function");
}