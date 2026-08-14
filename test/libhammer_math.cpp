#include "hammer_math.hpp"
#include <stdint.h>

// Provide the actual implementations
template <int A, int B> int Mult<A, B>::math() { return A * B; }
template <int A, int B> int Add<A, B>::math() { return A + B; }

// A volatile sink to completely defeat the -O2 constant-folding optimizer
volatile uintptr_t compiler_defeat_sink = 0;

// 1. Process a single row (Max Depth: YSIZE)
template<int A, int B>
struct ForceRow {
    static void run() {
        // Taking the address of the function strictly forces the compiler 
        // to instantiate it and export it to the ELF symbol table!
        compiler_defeat_sink += reinterpret_cast<uintptr_t>(&Mult<A, B>::math);
        compiler_defeat_sink += reinterpret_cast<uintptr_t>(&Add<A, B>::math);
        ForceRow<A, B-1>::run();
    }
};

template<int A>
struct ForceRow<A, -1> {
    static void run() { }
};

// 2. Process the grid by summing rows (Max Depth: XSIZE)
template<int A>
struct ForceGrid {
    static void run() {
        ForceRow<A, YSIZE>::run();
        ForceGrid<A-1>::run();
    }
};

template<>
struct ForceGrid<-1> {
    static void run() { }
};

// Run this automatically when the library is dlopen'd to ensure the sink is evaluated
extern "C" __attribute__((constructor)) void generate_all_symbols() {
    ForceGrid<XSIZE>::run();
}