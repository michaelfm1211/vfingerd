#include "clients.h"

#include <arpa/inet.h>
#include <ev.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "ev.h"
#include "server.h"
#include "util.h"

// Logs a request to stdout in the format of "[time] ipaddr    query"
static void log_query(const char *ipaddr, const char *query) {
  if (*query == '\0') query = "ALL";

  time_t raw_time = time(NULL);
  if (raw_time == (time_t)-1) return;
  struct tm curr_time;
  if (!localtime_r(&raw_time, &curr_time)) return;
  char timestr[27];
  strftime(timestr, 27, "%d/%b/%Y:%H:%M:%S %z", &curr_time);

  printf("[%s] %s\t%s\n", timestr, ipaddr, query);
}

// Resolves a username to a config_ent
static struct config_ent *resolveUsername(struct config_ent *config,
                                          const char *uname) {
  while (config != NULL) {
    if (!strcmp(config->name, uname)) return config;
    config = config->next;
  }
  return NULL;
}

// Read from the client and set client->query to the config_ent that is
// requested.
static void read_query(struct ev_loop *loop, struct client *client) {
  char query[BUF_SIZ];
  read(client->fd, query, BUF_SIZ - 1);

  char *cr = strstr(query, "\r\n");
  if (cr == NULL) {
    client->error = BAD_QUERY;
    goto error;
  }
  *cr = '\0';

  // don't print if it could contain dangerous characters
  if (!validUsername(query)) {
    client->error = UNKNOWN_USER;
    goto error;
  }
  log_query(client->ipaddr, query);

  if (*query == '\0') {
    client->state = WRITE_ALL;
    goto end;
  }

  client->query = resolveUsername(client->server->config, query);
  if (client->query->aliasOf)
    client->query =
        resolveUsername(client->server->config, client->query->aliasOf);

  if (client->query == NULL) {
    client->error = UNKNOWN_USER;
    goto error;
  } else
    client->state = WRITE_PLAN;

  goto end;
error:
  client->state = WRITE_ERROR;
end:
  ev_io_stop(loop, &client->io);
  ev_io_init(&client->io, client_cb, client->fd, EV_WRITE);
  ev_io_start(loop, &client->io);
}

// Write a list of all non-hidden users to the client
static void write_all(struct client *client) {
  const char *header = USER_LIST_HEADER "\r\n";
  const char *footer = USER_LIST_FOOTER " " SERVER_SIG "\r\n";
  size_t header_len = strlen(header);
  size_t footer_len = strlen(footer);

  size_t buf_len = header_len + footer_len + BUF_SIZ + 1;
  char *buf = malloc(buf_len);
  strcpy(buf, header);

  size_t len = 0;
  struct config_ent *ptr = client->server->config;
  while (ptr != NULL) {
    if (ptr->hidden) goto cont;

    size_t name_len = strlen(ptr->name) + 2;
    if (name_len + len > buf_len - header_len - footer_len - 1) {
      buf_len += BUF_SIZ;
      char *newbuf = realloc(buf, buf_len);
      if (newbuf == NULL) {
        free(buf);
        client->error = SERVER;
        client->state = WRITE_ERROR;
        return;
      }
      buf = newbuf;
      continue;  // do not increment
    }

    strcat(buf, ptr->name);
    strcat(buf, "\r\n");
    len += name_len;
  cont:
    ptr = ptr->next;
  }

  strcat(buf, footer);
  write(client->fd, buf, header_len + footer_len + len);
  client->state = DISCONNECT;
  free(buf);
}

// Write the user's plan to the client
static void write_plan(struct client *client) {
  size_t buf_len;
  char *buf;

  if (client->query->plan == NULL) {
    buf_len = strlen(client->query->name) + strlen(client->query->real_name) +
              sizeof(NO_PLAN) - 1 + sizeof(SERVER_SIG) - 1 + 30;
    buf = malloc(buf_len);
    snprintf(buf, buf_len,
             "Username: %s\nReal Name: %s\r\n\r\n" NO_PLAN " " SERVER_SIG
             "\r\n",
             client->query->name, client->query->real_name);
  } else {
    buf_len = strlen(client->query->name) + strlen(client->query->real_name) +
              sizeof(PLAN_HEADER) - 1 + strlen(client->query->plan) +
              sizeof(PLAN_FOOTER) - 1 + sizeof(SERVER_SIG) - 1 + 34;
    buf = malloc(buf_len);
    snprintf(buf, buf_len,
             "Username: %s\nReal Name: %s\r\n\r\n" PLAN_HEADER
             "\r\n%s\r\n" PLAN_FOOTER " " SERVER_SIG "\r\n",
             client->query->name, client->query->real_name,
             client->query->plan);
  }
  write(client->fd, buf, buf_len - 1);
  client->state = DISCONNECT;
  free(buf);
}

// Write an error to the client.
static void write_error(struct client *client) {
  char *msg = NULL;
  switch (client->error) {
    case BAD_QUERY:
      msg = ERR_BAD_QUERY " " SERVER_SIG "\r\n";
      break;
    case NONE:
      msg = ERR_UNKNOWN " " SERVER_SIG "\r\n";
      break;
    case SERVER:
      msg = ERR_SERVER " " SERVER_SIG "\r\n";
      break;
    case UNKNOWN_USER:
      msg = ERR_UNKNOWN_USER " " SERVER_SIG "\r\n";
      break;
  }
  write(client->fd, msg, strlen(msg));
  client->state = DISCONNECT;
}

void client_disconnect(struct ev_loop *loop, struct client *client) {
  ev_timer_stop(loop, &client->timeout.timer);
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
      client_disconnect(loop, client);
      break;
  }
}
