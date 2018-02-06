#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// TODO: Implement your signal handling code here!
__attribute__((constructor)) void init() {
  printf("This code runs at program startup\n");
}
