#include "audit_hook.hpp"
#include "hammer_math.hpp"
#include <string>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cstdint>

// The unique wrapper generator
template<int A, int B>
struct HookGen {
    static void* mult_orig;
    static int MultTramp() { return ((int(*)())mult_orig)() * -1; }
    
    static void* add_orig;
    static int AddTramp() { return ((int(*)())add_orig)() * -1; }

    static void reg(bool hook_mult) {
        std::string sm = "_ZN4MultILi" + std::to_string(A) + "ELi" + std::to_string(B) + "EE4mathEv";
        std::string sa = "_ZN3AddILi" + std::to_string(A) + "ELi" + std::to_string(B) + "EE4mathEv";
        if (hook_mult) {
            ah_register_hook("hammer", sm.c_str(), reinterpret_cast<void*>(MultTramp), &mult_orig, nullptr, false);
        } else {
            ah_register_hook("hammer", sa.c_str(), reinterpret_cast<void*>(AddTramp), &add_orig, nullptr, false);
        }
    }
};

// Allocate the 3,362 unique state pointers
template<int A, int B> void* HookGen<A,B>::mult_orig = nullptr;
template<int A, int B> void* HookGen<A,B>::add_orig = nullptr;

// 2D Template Recursion for Registration
template<int A, int B>
struct RegRow {
    static void do_it(bool hook_mult) {
        HookGen<A, B>::reg(hook_mult);
        RegRow<A, B-1>::do_it(hook_mult);
    }
};

template<int A>
struct RegRow<A, -1> {
    static void do_it(bool) { }
};

template<int A>
struct RegGrid {
    static void do_it(bool hook_mult) {
        RegRow<A, YSIZE>::do_it(hook_mult);
        RegGrid<A-1>::do_it(hook_mult);
    }
};

template<>
struct RegGrid<-1> {
    static void do_it(bool) { }
};

// The la_objopen callback
extern "C" void ah_plugin_on_objopen(const char* libname, uintptr_t cookie) {
    if (strstr(libname, "libhammer_math")) {
        const char* mode = getenv("HAMMER_MODE");
        if (mode && strcmp(mode, "mult") == 0) {
            std::cerr << "DIAGNOSTIC: Plugin registering MULT hooks for " << libname << "\n";
            RegGrid<XSIZE>::do_it(true);
        } else if (mode && strcmp(mode, "add") == 0) {
            std::cerr << "DIAGNOSTIC: Plugin registering ADD hooks for " << libname << "\n";
            RegGrid<XSIZE>::do_it(false);
        }
    }
}