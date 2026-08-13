#include "audit_hook.hpp"

int (*real_return_five)() = nullptr;
int (*real_return_six)() = nullptr;

int wrong_return_four() { return 5; }

int wrong_return_five() { return 4; }

int wrong_return_six() { return 3; }

__attribute__((constructor)) void init() {
  // 1. Unfiltered hook (Defaults to AH_FILTER_GLOBAL)
  // We use isolated tool names so filters don't bleed across hooks!
  audit_hooks::register_replace<wrong_return_four>("test_filter_four",
                                                   "return_four");

  // 2. Filtered hooks using the new modern C++ overload and proper enums
  const char *libs[] = {"libnum3.so"};

  audit_hooks::register_wrap<wrong_return_five, &real_return_five>(
      "test_filter_five", "return_five");
  // Pass the tool name, the enum mode, and the array
  ah_set_caller_filter("test_filter_five", AH_FILTER_INCLUDE, libs);

  audit_hooks::register_wrap<wrong_return_six, &real_return_six>(
      "test_filter_six", "return_six");
  ah_set_caller_filter("test_filter_six", AH_FILTER_INCLUDE, libs);
}