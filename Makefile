PREFIX=/usr/local

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

CFLAGS += -Wall -Wextra -std=c99 -pedantic
LDFLAGS += -lev

all: CFLAGS += -c
all: vfingerd docs

.PHONY: docs
docs: vfingerd.1

%.o: %.c
	@echo [CC] $@
	@$(CC) $(CFLAGS) $< -o $@

vfingerd: $(OBJS)
	@echo [LD] $@
	@$(CC) $(LDFLAGS) $^ -o vfingerd

vfingerd.1: vfingerd.1.scd
	@echo [SCDOC] $@
	@scdoc < $^ > $@

.PHONY: dev
dev: CFLAGS += -g -fsanitize=address
dev: $(SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o vfingerd-dev

.PHONY: install
install: all
	mkdir -p "${PREFIX}/bin"
	cp vfingerd "${PREFIX}/bin"
	chmod 755 "${PREFIX}/bin/vfingerd"
	mkdir -p "${PREFIX}/share/man/man1"
	cp vfingerd.1 "${PREFIX}/share/man/man1/vfingerd.1"
	chmod 644 "${PREFIX}/share/man/man1/vfingerd.1"

clean:
	rm -rf $(OBJS) vfingerd-dev vfingerd-dev.dSYM vfingerd
