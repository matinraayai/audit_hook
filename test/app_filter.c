#include <stdio.h>

/* Function declarations matching num.h */
extern int return_four();
extern int return_five();
extern int return_six();

int main() {
  int result;
  int had_error = 0;

  /* return_four is wrapped globally without a filter.
     Expected to return the wrapper's value: 5. */
  result = return_four();
  if (result != 5) {
    fprintf(stderr, "ERROR: wrapper function should return 5\n");
    had_error = -1;
  }

  /* return_five is filtered to only apply if called from libnum3.so.
     Since we are calling it from the main executable, it should bypass
     the wrapper and return the original library value: 5. */
  result = return_five();
  if (result != 5) {
    fprintf(stderr,
            "ERROR: library function should return 5. no wrapping for exec\n");
    had_error = -1;
  }

  /* return_six is similarly filtered to bypass the executable.
     It should return the original library value: 6. */
  result = return_six();
  if (result != 6) {
    fprintf(stderr,
            "ERROR: library function should return 6 no wrapping for exec\n");
    had_error = -1;
  }

  if (had_error == 0) {
    printf("SUCCESS: All caller-based filter tests passed!\n");
  }

  return had_error;
}