#include <stdio.h>

extern int call_func_three_x();
extern int call_func_three_y();

int main() {
  int had_error = 0;
  int x_val = call_func_three_x();
  int y_val = call_func_three_y();

  // libX should hit the wrapper, which calls the replacement (100 + 1 = 101)
  if (x_val != 101) {
    fprintf(stderr, "ERROR: Expected x_val=101, got %d\n", x_val);
    had_error = 1;
  }

  // libY bypasses the wrapper and hits the replacement directly (100)
  if (y_val != 100) {
    fprintf(stderr, "ERROR: Expected y_val=100, got %d\n", y_val);
    had_error = 1;
  }

  if (!had_error) {
    printf("SUCCESS: Mixed divergence routed correctly!\n");
  }
  return had_error;
}