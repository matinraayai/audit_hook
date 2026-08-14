#include <stdio.h>

extern int call_func_two_x();
extern int call_func_two_y();

int main() {
  int had_error = 0;
  int x_val = call_func_two_x();
  int y_val = call_func_two_y();

  // Both should hit both wrappers seamlessly (20 + 1 + 1 = 22)
  if (x_val != 22 || y_val != 22) {
    fprintf(stderr, "ERROR: Expected 22 for both, got X:%d Y:%d\n", x_val, y_val);
    had_error = 1;
  }

  if (!had_error) {
    printf("SUCCESS: Pure static chain executed correctly!\n");
  }
  return had_error;
}