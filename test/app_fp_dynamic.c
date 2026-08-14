#include "audit_hook_dynamic.h"
#include <stdio.h>

// Target function from libdispatch_target.so (Returns 30 natively)
extern int func_three();

// The application defines its own replacement hook just like GOTCHA!
int app_defined_replacement() {
  return 42;
}

int main() {
  int had_error = 0;

  // ld.so resolves this ONCE at load-time to our Dynamic Dispatcher thunk
  int (*fp)(void) = &func_three;

  // 1. First Call: "tool_dyn" was EXCLUDED by the plugin on startup.
  // Dispatcher routes directly to the native OS function (Native 30)
  int result1 = fp();
  if (result1 != 30) {
    fprintf(stderr, "ERROR: Expected native 30, got %d\n", result1);
    had_error = 1;
  }

  // 2. THE RUNTIME TARGET TOGGLE (GOTCHA STYLE)
  // Cross the namespace boundary and dynamically bind our own local function pointer!
  // This implicitly converts the hook to AH_FILTER_GLOBAL and overrides the plugin's target.
  if (ah_set_target("tool_dyn", (void *)&app_defined_replacement) != 0) {
    fprintf(stderr, "ERROR: Failed to set new dynamic target.\n");
    had_error = 1;
  }

  // 3. Second Call: The GOT pointer hasn't changed, but the dynamic dispatcher
  // now routes straight into our application-defined function!
  int result2 = fp();
  if (result2 != 42) {
    fprintf(stderr, "ERROR: Expected hooked 42, got %d\n", result2);
    had_error = 1;
  }

  if (!had_error) {
    printf("SUCCESS: Function pointer toggled via ah_set_target!\n");
  }
  return had_error;
}