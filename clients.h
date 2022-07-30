// clients.h - Handles the client connection.

#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <ev.h>
#include "config.h"

enum client_state {
	READ_QUERY,
	WRITE_RESP,
	WRITE_ERROR,
	DISCONNECT
};

enum client_error {
	NONE,
	UNKNOWN_USER
};

struct client {
	ev_io io;
	struct server *server;
	enum client_state state;
	struct config_ent *query;
	enum client_error error;
	int fd;
};

// Callback for events on a client socket
void client_cb(struct ev_loop *loop, ev_io *watcher, int revents);

