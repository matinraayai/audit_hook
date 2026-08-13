#include <stdio.h>

extern int func_w_r();
extern int func_r_w();
extern int func_w_w();
extern int func_r_r();

int main() {
    int had_error = 0;

    // Rule 1: Wrap -> Replace (Wrapper discarded, returns 100)
    if (func_w_r() != 100) {
        fprintf(stderr, "ERROR: Wrap->Replace failed. Expected 100, got %d\n", func_w_r());
        had_error = 1;
    }

    // Rule 2: Replace -> Wrap (Chained, Replace returns 200, Wrap adds 1 = 201)
    if (func_r_w() != 201) {
        fprintf(stderr, "ERROR: Replace->Wrap failed. Expected 201, got %d\n", func_r_w());
        had_error = 1;
    }

    // Rule 3: Wrap -> Wrap (Chained, Orig returns 30, Wrap1 adds 1, Wrap2 adds 10 = 41)
    if (func_w_w() != 41) {
        fprintf(stderr, "ERROR: Wrap->Wrap failed. Expected 41, got %d\n", func_w_w());
        had_error = 1;
    }

    // Rule 4: Replace -> Replace (Old discarded, returns 4000)
    if (func_r_r() != 4000) {
        fprintf(stderr, "ERROR: Replace->Replace failed. Expected 4000, got %d\n", func_r_r());
        had_error = 1;
    }

    if (had_error == 0) {
        printf("SUCCESS: All composition semantic tests passed!\n");
    }
    return had_error;
}