#include <arpa/inet.h>
#include <ev.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "server.h"
#include "util.h"

static void sigint_cb(struct ev_loop *loop, ev_signal *watcher, int revents) {
  UNUSED(watcher);
  UNUSED(revents);
  printf("Received SIGINT. Stopping server\n");
  ev_break(loop, EVBREAK_ALL);
}

static void sigpipe_cb(struct ev_loop *loop, ev_signal *watcher, int revents) {
  UNUSED(watcher);
  UNUSED(revents);
  UNUSED(loop);
}

void usage(void) {
  fprintf(stderr, "usage: vfingerd [-h] [-a addr] [-p port] [-c config]\n");
  exit(1);
}

int main(int argc, char *argv[]) {
  struct in_addr addr = {.s_addr = INADDR_ANY};
  int port = 79;
  char *config_path = DEF_CONFIG;

  int ch;
  while ((ch = getopt(argc, argv, ":a:p:c:")) != -1) {
    switch (ch) {
      case 'a':
        if (inet_aton(optarg, &addr) == 0) {
          fprintf(stderr, "error: Invalid address '%s'\n", optarg);
          return 1;
        }
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case 'c':
        config_path = optarg;
        break;
      case 'h':
      case ':':
      case '?':
        usage();
    }
  }

  if (optind < argc) usage();

  struct config_ent *config = config_parse(config_path);

  struct ev_loop *loop = EV_DEFAULT;

  struct server serv;
  server_create(&serv, config, &addr, port);
  ev_io_start(loop, &serv.io);

  ev_signal sigint_watcher;
  ev_signal_init(&sigint_watcher, sigint_cb, SIGINT);
  ev_signal_start(loop, &sigint_watcher);

  ev_signal sigpipe_watcher;
  ev_signal_init(&sigpipe_watcher, sigpipe_cb, SIGPIPE);
  ev_signal_start(loop, &sigpipe_watcher);

  printf("Starting server on port %d\n", port);
  ev_run(loop, 0);

  config_free(config);
  shutdown(serv.fd, SHUT_RDWR);
  close(serv.fd);
  return 0;
}
