#include "util.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void error(const char *msg) {
  perror(msg);
  exit(1);
}

bool validUsername(const char *uname) {
  if (*uname == '\0') return true;
  if (strlen(uname) > 32) return false;
  if (!islower(*uname) && *uname != '_') return false;
  while (*uname) {
    if (*uname == '$' && *(uname + 1) == '\0') break;
    if (!islower(*uname) && !isdigit(*uname) && *uname != '-' && *uname != '_')
      return false;
    uname++;
  }
  return true;
}
