#include "definitions.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int *state = 0;
int ptr = 0;

int startVerifyState(unsigned char *c, int *state, int *DONE) {
  switch (*state) {
  case 0: // expecting flag
    if (*c == FLAG) {
      *state = 1;
    } // else stay in same state
    break;
  case 1: // expecting A
    if (*c == A) {
      *state = 2;
    } else if (*c != FLAG) { // if not FLAG instead of A
      *state = 0;
    } // else stay in same state
    break;
  case 2: // Expecting C_SET
    if (*c == C_SET) {
      *state = 3;
    } else if (*c == FLAG) { // if FLAG received
      *state = 1;
    } else { // else go back to beggining
      *state = 0;
    }
    break;
  case 3: // Expecting BCC
    if (*c == UA_BCC) {
      *state = 4;
    } else {
      *state = 0; // else go back to beggining
    }
    break;
  case 4: // Expecting FLAG
    if (*c == FLAG) {
      *state = 5;

    } else {
      *state = 0; // else go back to beggining
    }
    break;
  }
  *DONE = TRUE;
  alarm(0);
  printf("Received UA properly\n");
  return 1;
}