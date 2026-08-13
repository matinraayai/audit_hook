#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>

#define LIB_NAME "./.libs/libnum.so"
#define LIB2_NAME "./.libs/libnum2.so"

int main() {
    void *libnum;
    void *libnum2;
    int (*retfour)(void);
    int (*retsix)(void);
    int (*test_retfive)(void);
    int (*retdummy)(void);
    int had_error = 0;

    /* Load the first libnum.so */
    libnum = dlopen(LIB_NAME, RTLD_NOW);
    if (!libnum) {
        fprintf(stderr, "ERROR: Test failed to dlopen libnum.so with %s\n", dlerror());
        return -1;
    }

    /* Check if return_four is wrapped from libnum.so */
    retfour = (int (*)(void))dlsym(libnum, "return_four");
    if (retfour == NULL || retfour() != 4) {
        fprintf(stderr, "ERROR: dlsym returned original function, not wrapped from libnum.so\n");
        had_error = -1;
    }

    /* Test 2: Does a call in a dlopen'd library get rerouted */
    test_retfive = (int (*)(void))dlsym(libnum, "test_return_five");
    if (test_retfive == NULL || test_retfive() != 5) {
        fprintf(stderr, "ERROR: call to return_five in libnum.so was not wrapped by correct_return_five\n");
        had_error = -1;
    }

    /* Load libnum2.so */
    libnum2 = dlopen(LIB2_NAME, RTLD_NOW);
    if (!libnum2) {
        fprintf(stderr, "ERROR: Test failed to dlopen libnum2.so with %s\n", dlerror());
        return -1;
    }

    /* Check if return_six is wrapped from libnum2.so */
    retsix = (int (*)(void))dlsym(libnum2, "return_six");
    if (retsix == NULL || retsix() != 6) {
        fprintf(stderr, "ERROR: dlsym returned original function, not wrapped from libnum2.so\n");
        had_error = -1;
    }

    /* Check RTLD_DEFAULT */
    retfour = (int (*)(void))dlsym(RTLD_DEFAULT, "return_four");
    if (retfour == NULL || retfour() != 4) {
        fprintf(stderr, "ERROR: call to return_four should be found in RTLD_DEFAULT and return 4\n");
        had_error = -1;
    }

    test_retfive = (int (*)(void))dlsym(RTLD_DEFAULT, "test_return_five");
    if (test_retfive == NULL || test_retfive() != 5) {
        fprintf(stderr, "ERROR: call to test_return_five in RTLD_DEFAULT failed\n");
        had_error = -1;
    }

    retdummy = (int (*)(void))dlsym(RTLD_DEFAULT, "return_dummy");
    if (retdummy != NULL) {
        fprintf(stderr, "ERROR: call to return_dummy should not be found in RTLD_DEFAULT\n");
        had_error = -1;
    }

    if (had_error == 0) {
        printf("SUCCESS: All dlopen and dlsym tests passed!\n");
    }

    return had_error;
}