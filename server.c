#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <ev.h>
#include "clients.h"
#include "config.h"
#include "server.h"
#include "util.h"

void server_cb(struct ev_loop *loop, ev_io *watcher, int revents) {
	UNUSED(revents);
	struct server *serv = (struct server *)watcher;
	while (1) {
		int client_fd = accept(serv->fd, NULL, NULL);
		if (client_fd == -1) {
			if (errno != EAGAIN)
				perror("accept()");
			break;
		}

		struct client *client = &(serv->clients[serv->num_conn++]);
		client->server = serv;
		client->state = READ_QUERY;
		client->query = NULL;
		client->error = NONE;
		client->fd = client_fd;
		int flags = fcntl(client->fd, F_GETFL) | O_NONBLOCK;
		if (fcntl(client->fd, F_SETFL, flags) == -1) {
			perror("fcntl()");
			ev_break(loop, EVBREAK_ALL);
		}

		// TODO: enforce a maximum connection time for clients with
		// ev_timer
		ev_io_init(&client->io, client_cb, client->fd, EV_READ);
		ev_io_start(loop, &client->io);
	}
}

void server_create(struct server *serv, struct config_ent *config, struct
		in_addr *addr, int port) {
	serv->num_conn = 0;
	serv->config = config;

	serv->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (serv->fd == -1)
		error("socket()");
	
	if (setsockopt(serv->fd, SOL_SOCKET, SO_REUSEADDR, &(int){1},
			sizeof(int)) == -1)
		error("setsockopt()");

	int flags = fcntl(serv->fd, F_GETFL) | O_NONBLOCK;
	if (fcntl(serv->fd, F_SETFL, flags) == -1)
		error("fcntl()");

	serv->addr.sin_family = AF_INET;
	serv->addr.sin_addr = *addr;
	serv->addr.sin_port = htons(port);

	if (bind(serv->fd, (struct sockaddr *)&serv->addr, sizeof(serv->addr)) ==
			-1)
		error("bind()");

	if (listen(serv->fd, MAX_CLIENTS) == -1)
		error("listen()");

	ev_io_init(&serv->io, server_cb, serv->fd, EV_READ);
}

