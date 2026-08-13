#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  // Note: In an uninstalled libtool tree, objects are in .libs/
  void *handle = dlopen("./.libs/libdummy.so", RTLD_NOW);
  if (!handle) {
    fprintf(stderr, "Failed to load dummy lib: %s\n", dlerror());
    return 1;
  }

  // Attempt to manually look up the function pointer
  int (*func)(int) = dlsym(handle, "target_function");
  if (func) {
    func(42);
  }
  return 0;
}