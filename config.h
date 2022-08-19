// config.h - Parses the configuration file.

#pragma once

#include <stdbool.h>
#include <stddef.h>

// Represents an entry in the configuration file. Additionally, each
// configuration entry represents one vfingerd user.
struct config_ent {
	const char *name;
	const char *real_name;
	const char *plan;
	struct config_ent *next;
};

// Returns a linked list of parsed configuration entries.
struct config_ent *config_parse(const char *path);

// Free a linked list of configuration entries, like one that is returned
// from parse_config().
void config_free(struct config_ent *ent);

