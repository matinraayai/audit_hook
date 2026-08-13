#include "audit_hook.hpp"

// We must declare the extern C function if it's not wrapped by a static 
// template in your current audit_hook.hpp yet.
extern "C" int ah_set_caller_filter(const char* tool_name, const char* symbol_name, const char* library_name);

int (*real_return_five)() = nullptr;
int (*real_return_six)() = nullptr;

// Replaces completely (No filtering)
int wrong_return_four() { 
    return 5; 
}

// Emulates gotcha_filter_libraries_by_name("libnum3.so")
int wrong_return_five() {
    return 4; 
}

// Emulates gotcha_only_filter_last()
int wrong_return_six() {
    return 3; 
}

__attribute__((constructor)) void init() {
    // 1. Unfiltered hook
    audit_hooks::register_replace<wrong_return_four>("test_filter", "return_four");
    
    // 2. Filtered hooks
    audit_hooks::register_wrap<wrong_return_five, &real_return_five>("test_filter", "return_five");
    ah_set_caller_filter("test_filter", "return_five", "libnum3.so");

    audit_hooks::register_wrap<wrong_return_six, &real_return_six>("test_filter", "return_six");
    ah_set_caller_filter("test_filter", "return_six", "libnum3.so");
}