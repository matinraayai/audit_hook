int return_five() { return 4; }

int return_four() {
  /* Intentional bug, wrapping will correct this to return 4 */
  return 3;
}

int test_return_five() { return return_five(); }