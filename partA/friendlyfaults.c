#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>


void seg_handler(int signal, siginfo_t* info, void* ctx) {
  if (signal == 11) {
    char messages[8][128] = {"Cause of death: segfault.",
                             "Segfault, but it's alright! You'll get it to "
                             "work eventually (maybe after the deadline, but "
                             "hey, at least it works?).",
                             "Segfault, but you got this, bro.",
                             "Congrats, it's going to be 10 hours before this "
                             "program is fixed.",
                             "Segfault, but I believe in you! You can fix this!"
                             " (doubtful, tbh)",
                             "Are you a CS major? Wouldn't've guessed it. Jk "
                             "welcome to the never-ending cycle of suffering, "
                             "fellow egomaniac.",
                             "Segfault, but you can fix it. You're a passable"
                             " programmer (probably... you did get this far, "
                             "after all).",
                             "Segfault: RIP. Sorry. gg. Maybe memes will "
                             "help?"};
    srand(time(0));
    printf("%s\n", messages[rand() % 8]);
    exit(EXIT_FAILURE);
  }
}


__attribute__((constructor)) void init() {
  // Structure taken from lab page:
  // create sigaction handler:
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_sigaction = seg_handler;
  sa.sa_flags = SA_SIGINFO;
  
  // Check for errors:
  if(sigaction(SIGSEGV, &sa, NULL) != 0) {
    perror("sigaction failed");
    exit(2);
  }
}

