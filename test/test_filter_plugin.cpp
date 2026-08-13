#include "audit_hook.hpp"

int (*real_return_five)() = nullptr;
int (*real_return_six)() = nullptr;
int (*real_return_six_again)() = nullptr;

int wrong_return_four() { return 5; }

int wrong_return_five() { return 4; }

int wrong_return_six() { return 3; }

// Emulates the GOTCHA intent of wrapping a function a second time.
int wrong_return_six_again() {
  // Passes the call down the chain
  return real_return_six_again ? real_return_six_again() : 0;
}

__attribute__((constructor)) void init() {
  // 1. Unfiltered hook (Defaults to AH_FILTER_GLOBAL)
  audit_hooks::register_replace<wrong_return_four>("test_filter_four",
                                                   "return_four");

  // 2. Filtered hooks using the new modern C++ overload
  const char *libs[] = {"libnum3.so"};

  // Tests the "Global to Include" transition rule
  audit_hooks::register_wrap<wrong_return_five, &real_return_five>(
      "test_filter_five", "return_five");
  ah_set_caller_filter("test_filter_five", AH_FILTER_INCLUDE, libs);

  // Tests the "Global to Include" transition rule
  audit_hooks::register_wrap<wrong_return_six, &real_return_six>(
      "test_filter_six", "return_six");
  ah_set_caller_filter("test_filter_six", AH_FILTER_INCLUDE, libs);

  // 3. Emulate GOTCHA's final gotcha_wrap(func_six, 1, NULL);
  // This tests the "Wrap followed by Wrap (Chaining)" composition rule!
  audit_hooks::register_wrap<wrong_return_six_again, &real_return_six_again>(
      "test_filter_six_again", "return_six");

  // We must also apply the INCLUDE filter to this outer wrapper. Because the
  // linker resolves the binding to the outermost trampoline, leaving this as
  // GLOBAL would intercept the main application's call and bypass the inner
  // hook's linker-level filter.
  ah_set_caller_filter("test_filter_six_again", AH_FILTER_INCLUDE, libs);
}