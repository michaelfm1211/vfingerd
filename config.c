#include <ctype.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "config.h"
#include "util.h"

struct debug_info {
	const char *path;
	int linenum;
} debug;

// Strips all whitespace from a string
static void strip_whitespace(char *str) {
	int x = 0;
	char quoteChar = 0;

	for (int i = 0; str[i]; i++) {
	if (str[i] == '\'' || str[i] == '"') {
		if (!quoteChar) {
			quoteChar = str[i];
			continue;
		} else if (str[i] == quoteChar) {
			quoteChar = 0;
			continue;
		}
	}

        if (quoteChar || !isspace(str[i]))
            str[x++] = str[i];
    }
    str[x] = '\0';
}

// Rewrite all LF characters in a buffer to CRLFs.
static void rewrite_lf_crlf(char **buf) {
	UNUSED(buf);
	// TODO: finish
}

// Add a new node to the end of a linked list, or initalize the list if NULL
static void list_add_node(struct config_ent **list, struct config_ent **tail) {
	struct config_ent *new = calloc(1, sizeof(struct config_ent));
	if (*list == NULL)
		*list = *tail = new;
	else {
		(*tail)->next = new;
		*tail = new;
	}
}

// Read a plan file and populate a configuration entry's plan with its
// contents.
static bool list_node_plan_file(struct config_ent *node, const char *path,
		bool suppress_errors) {
	FILE *plan = fopen(path, "r");
	if (plan == NULL) {
		if (!suppress_errors) {
			fprintf(stderr, "%s: Error opening file on line %d\n",
					debug.path, debug.linenum);
			perror(path);
		}
		return false;
	}
	fseek(plan, 0, SEEK_END);
	long plan_len = ftell(plan);
	rewind(plan);
	
	char *buf = malloc(plan_len + 1);
	fread(buf, plan_len, 1, plan);
	buf[plan_len] = '\0';

	if (strstr(buf, "\r\n") == NULL)
		rewrite_lf_crlf(&buf);

	node->plan = buf;
	fclose(plan);
	return true;
}

// Populate a configuration entry with information from /etc/passwd
static bool list_node_use_passwd(const char *name, struct config_ent *node) {
	struct passwd *pw = getpwnam(name);
	if (pw == NULL)
		return false;
	node->real_name = strdup(pw->pw_gecos);

	if (pw->pw_dir != NULL) {
		char plan_path[strlen(pw->pw_dir) + 7];
		sprintf(plan_path, "%s/.plan", pw->pw_dir);
		if (!list_node_plan_file(node, plan_path, true))
			node->plan = NULL;
	} else
		node->plan = NULL;

	return true;
}

// Set a key to a value on linked list a node, or return false if the key is
// invalid or node is NULL. value should be allocated and not freed after
// passed to this function.
static bool list_node_set(struct config_ent *node, const char *key, const char
		*value) {
	if (node == NULL) {
		fprintf(stderr, "%s: All key/value pairs must belong to a "
				"section\n", debug.path);
		goto error;
	}

	if (!strcmp(key, "use_passwd")) {
		if (!strcmp(value, "true")) {
			if (!list_node_use_passwd(node->name, node)) {
				fprintf(stderr, "%s: Could not get "
						"information for user '%s'"
						" on line %d\n", debug.path,
						node->name, debug.linenum);
				goto error;
			}
			free((void *)value);
		} else if (strcmp(value, "false")) {
			fprintf(stderr, "%s: Invalid value for key '%s' on "
					"line %d\n", debug.path, key,
					debug.linenum);
			goto error;
		}
	} else if (!strcmp(key, "hidden")) {
		if (!strcmp(value, "true")) {
			node->hidden = true;
		} else if (strcmp(value, "false")) {
			fprintf(stderr, "%s: Invalid value for key '%s' on "
					"line %d\n", debug.path, key,
					debug.linenum);
			goto error;
		}
	} else if (!strcmp(key, "name")) {
		node->real_name = value;
	} else if (!strcmp(key, "plan")) {
		char *tmp = malloc(strlen(value) + 3);
		sprintf(tmp, "%s\r\n", value);
		node->plan = tmp;
		free((void *)value);
	} else if (!strcmp(key, "plan_file")) {
		if (!list_node_plan_file(node, value, false))
			goto error;
		free((void *)value);
	} else {
		fprintf(stderr, "%s: Unknown key '%s' on line %d\n",
				debug.path, key, debug.linenum);
		goto error;
	}
	return true;
error:
	free((void *)value);
	return false;
}

// Handle basic heredoc fucntionality. Returns an allocated value string or
// NULL if an error occurs.
static char *heredoc(FILE *file, const char *delim, struct debug_info *debug) {
	char full_delim[strlen(delim) + 2];
	strcpy(full_delim, delim);
	strcat(full_delim, "\n");

	char *buf = malloc(BUF_SIZ);
	size_t bufcap = BUF_SIZ, buflen = 0;
	
	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	while ((linelen = getline(&line, &linecap, file)) != -1) {
		if (!strncmp(line, full_delim, linelen-1))
			break;

		while (buflen+linelen >= bufcap-1) {
			bufcap += BUF_SIZ;
			char *newbuf = realloc(buf, bufcap);
			if (newbuf == NULL) {
				free(line);
				free(buf);
				perror("realloc()");
				return NULL;
			}
			buf = newbuf;
		}
		
		strcpy(buf+buflen, line);
		buflen += linelen;
		debug->linenum += 1;
	}
	free(line);
	buf[buflen] = '\0';
	return buf;
}

struct config_ent *config_parse(const char *path) {
	FILE *config = fopen(path, "r");
	if (config == NULL)
		error(path);

	debug.path = path;
	debug.linenum = 0;

	struct config_ent *list = NULL, *tail = list;

	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	while ((linelen = getline(&line, &linecap, config)) != -1) {
		debug.linenum++;
		if (linelen < 2 || line[0] == '#')
			continue;
		strip_whitespace(line);

		// Check for and handle a new entry
		if (line[0] == '[') {
			char *name = strdup(line + 1);
			name[linelen-3] = '\0';
			list_add_node(&list, &tail);
			tail->name = name;
			continue;
		}

		// Get a key/value pair
		char *value = NULL;
		char *key = strtok_r(line, "=", &value);
		if (value == NULL) {
			fprintf(stderr, "%s: Invalid key/value pair on line "
					"%d\n", path, debug.linenum);
			goto error;
		}
		
		// Check if value is a heredoc
		if (!strncmp(value, "<<", 2)) {
			char *delim = value+2;
			if ((value = heredoc(config, delim, &debug)) == NULL)
				goto error;
		} else
			value = strdup(value);

		if (list_node_set(tail, key, value) == false)
			goto error;
		
		continue;
	}
	free(line);
	fclose(config);
	return list;

error:
		free(line);
		config_free(list);
		fclose(config);
		exit(1);
}

void config_free(struct config_ent *ent) {
	if (ent == NULL)
		return;
	while (ent != NULL) {
		struct config_ent *next = ent->next;
		free((void *)ent->name);
		free((void *)ent->real_name);
		free((void *)ent->plan);
		free(ent);
		ent = next;
	}
}

