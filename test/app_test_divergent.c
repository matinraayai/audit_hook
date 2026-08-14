#include <stdio.h>

extern int call_func_one_x();
extern int call_func_one_y();

int main() {
  int had_error = 0;
  int x_val = call_func_one_x();
  int y_val = call_func_one_y();

  // libX should hit both wrappers (10 + 1 + 1 = 12)
  if (x_val != 12) {
    fprintf(stderr, "ERROR: Expected x_val=12, got %d\n", x_val);
    had_error = 1;
  }

  // libY should bypass Plugin B and only hit Plugin A (10 + 1 = 11)
  if (y_val != 11) {
    fprintf(stderr, "ERROR: Expected y_val=11, got %d\n", y_val);
    had_error = 1;
  }

  if (!had_error) {
    printf("SUCCESS: Divergent wrapper chain routed correctly!\n");
  }
  return had_error;
}