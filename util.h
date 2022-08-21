#pragma once

// default path for the configuration file
#define DEF_CONFIG "/etc/vfingerd.conf"
// size of arbitary buffers.
#define BUF_SIZ 1024
// signature of the server. only for comestics; appended to all responses
#define SERVER_SIG "michaelfm1211/vfingerd v0.2.0"
// maximum number of clients the server will listen to at once
#define MAX_CLIENTS 1024

#define UNUSED(x) (void)x

void error(const char *msg);

