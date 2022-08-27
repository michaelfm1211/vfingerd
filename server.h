// server.h - Handles server creation and accepting clients.

#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <ev.h>
#include "clients.h"
#include "config.h"
#include "util.h"

struct server {
	ev_io io;
	struct config_ent *config;
	int fd;
	struct sockaddr_in addr;
	int num_conn;
	struct client clients[MAX_CLIENTS];
};

// Create a server then listen for incoming clients.
void server_create(struct server *serv, struct config_ent *config, struct
		in_addr *addr, int port);

