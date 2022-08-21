#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ev.h>
#include "clients.h"
#include "config.h"
#include "server.h"
#include "util.h"

// Read from the client and set client->query to the config_ent that is
// requested.
static void read_query(struct ev_loop *loop, struct client *client) {
	char query[BUF_SIZ];
	read(client->fd, query, BUF_SIZ-1);

	char *cr = strstr(query, "\r\n");
	if (cr == NULL) {
		client->error = BAD_QUERY;
		client->state = WRITE_ERROR;
		goto end;
	}
	*cr = '\0';

	if (*query == '\0') {
		client->state = WRITE_ALL;
		goto end;
	}

	struct config_ent *ptr = client->server->config;
	while (ptr != NULL) {
		if (!strcmp(ptr->name, query)) {
			client->query = ptr;
			break;
		}
		ptr = ptr->next;
	}

	if (client->query == NULL) {
		client->error = UNKNOWN_USER;
		client->state = WRITE_ERROR;
	} else
		client->state = WRITE_PLAN;
end:
	ev_io_stop(loop, &client->io);
	ev_io_init(&client->io, client_cb, client->fd, EV_WRITE);
	ev_io_start(loop, &client->io);
}

// Write a list of all non-hidden users to the client
static void write_all(struct client *client) {
	const char *header = "--- Users List. ---\r\n";
	const char *footer = "--- End of Users List. --- "SERVER_SIG"\r\n";
	size_t header_len = strlen(header);
	size_t footer_len = strlen(footer);

	size_t buf_len = header_len + footer_len + BUF_SIZ + 1;
	char *buf = malloc(buf_len);
	strcpy(buf, header);

	size_t len = 0;
	struct config_ent *ptr = client->server->config;
	while (ptr != NULL) {
		if (ptr->hidden)
			goto cont;

		size_t name_len = strlen(ptr->name) + 2;
		if (name_len + len > buf_len-header_len-footer_len-1) {
			buf_len += BUF_SIZ;
			char *newbuf = realloc(buf, buf_len);
			if (newbuf == NULL) {
				free(buf);
				client->error = SERVER;
				client->state = WRITE_ERROR;
				return;
			}
			buf = newbuf;
			continue; // do not increment
		}

		strcat(buf, ptr->name);
		strcat(buf, "\r\n");
		len += name_len;
cont:
		ptr = ptr->next;
	}

	strcat(buf, footer);
	write(client->fd, buf, header_len+footer_len+len);
	client->state = DISCONNECT;
	free(buf);
}

// Write the user's plan to the client
static void write_plan(struct client *client) {
	size_t buf_len = strlen(client->query->name) +
		strlen(client->query->real_name) + strlen(SERVER_SIG) + 76;
	char *buf = malloc(buf_len);

	if (client->query->plan == NULL) {
		buf_len -= 29;
		sprintf(buf, "Username: %s\t\tReal Name: %s\r\n\r\n--- No "
				"Plan. --- %s\r\n", client->query->name,
				client->query->real_name, SERVER_SIG);
	} else {
		buf_len += strlen(client->query->plan);
		char *new_buf = realloc(buf, buf_len);
		if (new_buf == NULL)
			goto end;
		buf = new_buf;
		sprintf(buf, "Username: %s\t\tReal Name: %s\r\n\r\n--- Start "
				"of Plan.---\r\n%s\r\n--- End of Plan. --- "
				"%s\r\n", client->query->name,
				client->query->real_name, client->query->plan,
				SERVER_SIG);
	}
	write(client->fd, buf, buf_len-1);
end:
	client->state = DISCONNECT;
	free(buf);
}

// Write an error to the client.
static void write_error(struct client *client) {
	char *msg = NULL;
	switch (client->error) {
	case BAD_QUERY:
		msg = "--- Bad Query. ---"SERVER_SIG"\r\n";
		break;
	case NONE:
		msg = "--- An Unknown Error Occured. --- "SERVER_SIG"\r\n";
		break;
	case SERVER:
		msg = "--- A Server Error Occured. ---"SERVER_SIG"\r\n";
	case UNKNOWN_USER:
		msg = "--- No Such User. --- "SERVER_SIG"\r\n";
		break;
	}
	write(client->fd, msg, strlen(msg));
	client->state = DISCONNECT;
}

static void disconnect(struct ev_loop *loop, struct client *client) {
	ev_io_stop(loop, &client->io);
	shutdown(client->fd, SHUT_RDWR);
	close(client->fd);
	client->server->num_conn--;
}

void client_cb(struct ev_loop *loop, ev_io *watcher, int revents) {
	UNUSED(revents);
	struct client *client = (struct client *)watcher;

	switch (client->state) {
	case READ_QUERY:
		read_query(loop, client);
		break;
	case WRITE_ALL:
		write_all(client);
		break;
	case WRITE_PLAN:
		write_plan(client);
		break;
	case WRITE_ERROR:
		write_error(client);
		break;
	case DISCONNECT:
		disconnect(loop, client);
		break;
	}
}
