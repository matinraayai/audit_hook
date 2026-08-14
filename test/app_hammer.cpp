#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <stdbool.h>

int main() {
    // dlopen the checker library, which will automatically trigger the load of the math library
    void* handle = dlopen("./.libs/libhammer_check.so", RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        fprintf(stderr, "Failed to load libhammer_check.so: %s\n", dlerror());
        return 1;
    }

    typedef bool (*eval_func_t)(bool);
    eval_func_t verify_all_mults = (eval_func_t)dlsym(handle, "verify_all_mults");
    eval_func_t verify_all_adds = (eval_func_t)dlsym(handle, "verify_all_adds");

    if (!verify_all_mults || !verify_all_adds) {
        fprintf(stderr, "Failed to resolve evaluate functions\n");
        return 1;
    }

    const char* mode = getenv("HAMMER_MODE");
    bool is_mult_neg = (mode && strcmp(mode, "mult") == 0);
    bool is_add_neg = (mode && strcmp(mode, "add") == 0);

    int had_error = 0;

    if (!verify_all_mults(is_mult_neg)) {
        fprintf(stderr, "ERROR: Mult verification failed!\n");
        had_error = 1;
    }

    if (!verify_all_adds(is_add_neg)) {
        fprintf(stderr, "ERROR: Add verification failed!\n");
        had_error = 1;
    }

    if (!had_error) {
        printf("SUCCESS: All 3,362 functions evaluated perfectly!\n");
    }

    dlclose(handle);
    return had_error;
}