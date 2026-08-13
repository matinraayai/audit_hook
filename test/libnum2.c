int return_four() { return 6; }

int return_six() {
  /* Intentional bug, wrapping will correct this to return 6 */
  return 7;
}