#pragma once

#include <stdbool.h>

// default path for the configuration file
#define DEF_CONFIG "/etc/vfingerd.conf"
// size of arbitary buffers.
#define BUF_SIZ 1024
// signature of the server. only for comestics; appended to all responses
#define SERVER_SIG "michaelfm1211/vfingerd v0.3.0"
// maximum number of clients the server will listen to at once
#define MAX_CLIENTS 1024
// seconds since connection before server will terminate the connection
#define CONN_TIMEOUT 15.

#define UNUSED(x) (void)x

// Run perror() and exit with status code 1.
void error(const char *msg);

// Return true if uname 1) begins with a lowercase letter or underscore, 2)
// contains only lower case letters, digits, underscores, or dashes (except for
// the last character, which may also be a dollar sign), and 3) is less than 33
// characters long. Otherwise, returns false, unless uname starts with null.
bool validUsername(const char *uname);
