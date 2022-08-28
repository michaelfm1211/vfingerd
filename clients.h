// clients.h - Handles the client connection.

#pragma once

#include <ev.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "config.h"

#define USER_LIST_HEADER "--- Users List. ---"
#define USER_LIST_FOOTER "--- End of Users List. ---"

#define NO_PLAN "--- No Plan. ---"

#define PLAN_HEADER "--- Start of Plan. ---"
#define PLAN_FOOTER "--- End of Plan. ---"

#define ERR_BAD_QUERY "--- Bad Query. ---"
#define ERR_UNKNOWN "--- An Unknown Error Occured. ---"
#define ERR_SERVER "--- A Server Error Occured. ---"
#define ERR_UNKNOWN_USER "--- No Such User. ---"

enum client_state {
  READ_QUERY,
  WRITE_ALL,
  WRITE_PLAN,
  WRITE_ERROR,
  DISCONNECT
};

enum client_error { BAD_QUERY, NONE, SERVER, UNKNOWN_USER };

struct client_timeout {
  ev_timer timer;
  struct client *client;
};

struct client {
  ev_io io;
  char ipaddr[INET6_ADDRSTRLEN];
  struct client_timeout timeout;
  struct server *server;
  enum client_state state;
  struct config_ent *query;
  enum client_error error;
  int fd;
};

// Disconnect a client.
void client_disconnect(struct ev_loop *loop, struct client *client);

// Callback for events on a client socket
void client_cb(struct ev_loop *loop, ev_io *watcher, int revents);
