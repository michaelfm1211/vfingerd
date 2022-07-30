#include <stdio.h>
#include <stdlib.h>
#include "util.h"

void error(const char *msg) {
	perror(msg);
	exit(1);
}

