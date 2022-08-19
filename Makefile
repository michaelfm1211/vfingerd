PREFIX=/usr/local

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

CFLAGS += -Wall -Wextra -std=c99 -pedantic
LDFLAGS += -lev

all: CFLAGS += -c
all: vfingerd

%.o: %.c
	@echo [CC] $@
	@$(CC) $(CFLAGS) $< -o $@

vfingerd: $(OBJS)
	@echo [LD] $@
	@$(CC) $(LDFLAGS) $^ -o vfingerd

.PHONY: dev
dev: CFLAGS += -g -fsanitize=address
dev: $(SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o vfingerd-dev

.PHONY: install
install: all
	mkdir -p "${PREFIX}/bin"
	cp vfingerd "${PREFIX}/bin"
	chmod 755 "${PREFIX}/bin/vfingerd"

clean:
	rm -rf $(OBJS) vfingerd-dev vfingerd-dev.dSYM vfingerd
