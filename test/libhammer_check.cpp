#include "hammer_math.hpp"
#include <stdio.h>

template <int A, int B>
struct CheckerRow {
    static bool check_mult(bool is_neg) {
        int expected = A * B * (is_neg ? -1 : 1);
        int actual = Mult<A, B>::math(); // Forces cross-library PLT lookup!
        if (actual != expected) {
            fprintf(stderr, "DIAGNOSTIC: Mult<%d, %d>::math() returned %d, expected %d\n", A, B, actual, expected);
            return false;
        }
        return CheckerRow<A, B - 1>::check_mult(is_neg);
    }
    static bool check_add(bool is_neg) {
        int expected = (A + B) * (is_neg ? -1 : 1);
        int actual = Add<A, B>::math(); // Forces cross-library PLT lookup!
        if (actual != expected) {
            fprintf(stderr, "DIAGNOSTIC: Add<%d, %d>::math() returned %d, expected %d\n", A, B, actual, expected);
            return false;
        }
        return CheckerRow<A, B - 1>::check_add(is_neg);
    }
};

template <int A>
struct CheckerRow<A, -1> {
    static bool check_mult(bool) { return true; }
    static bool check_add(bool) { return true; }
};

template <int A>
struct CheckerGrid {
    static bool check_mult(bool is_neg) {
        if (!CheckerRow<A, YSIZE>::check_mult(is_neg)) return false;
        return CheckerGrid<A - 1>::check_mult(is_neg);
    }
    static bool check_add(bool is_neg) {
        if (!CheckerRow<A, YSIZE>::check_add(is_neg)) return false;
        return CheckerGrid<A - 1>::check_add(is_neg);
    }
};

template <>
struct CheckerGrid<-1> {
    static bool check_mult(bool) { return true; }
    static bool check_add(bool)  { return true; }
};

extern "C" bool verify_all_mults(bool is_neg) {
    return CheckerGrid<XSIZE>::check_mult(is_neg);
}

extern "C" bool verify_all_adds(bool is_neg) {
    return CheckerGrid<XSIZE>::check_add(is_neg);
}