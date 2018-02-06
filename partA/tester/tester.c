#include <stdio.h>

int main() {
  printf("This is a really awful program!\n");
  int* p = (int*)0x55;
  *p = 0;
  return 0;
}