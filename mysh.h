#ifndef _MYSHELL_H
#define _MYSHELL_H

#include <sys/queue.h>

/*
 *	Structure declarations
 */

STAILQ_HEAD(string_queue_head, string_queue_entry);

struct string_queue_entry {
	char *string;
	STAILQ_ENTRY(string_queue_entry) entries;
};

STAILQ_HEAD(cmd_queue_head, cmd_queue_entry);

struct cmd_queue_entry {
	struct command *cmdptr;
	STAILQ_ENTRY(cmd_queue_entry) entries;
};

struct command {
	char *name;
	char **args;
	int (*executioner)(char *name, char **args);
};

enum internal_cmd {
	CD,
	EXIT
};

/*
 *	Typedefs
 */

typedef struct string_queue_head StringQueueHead;

typedef struct string_queue_entry StringQueueEntry;

typedef struct cmd_queue_head CmdQueueHead;

typedef struct cmd_queue_entry CmdQueueEntry;

/*
 *	String queue function declarations
 */

StringQueueHead * initialize_string_queue(StringQueueEntry *e);

StringQueueEntry * create_string_queue_entry(char *s);

void insert_string_queue(StringQueueHead *hptr, StringQueueEntry *eptr);

void shallow_destroy_string_queue(StringQueueHead *qhptr);

char ** string_queue_to_array(StringQueueHead *qhptr);

/*
 *	Cmd queue function declarations
 */

CmdQueueHead * initialize_cmd_queue(CmdQueueEntry *e);

CmdQueueEntry * create_cmd_queue_entry(struct command *cmd);

void insert_cmd_queue(CmdQueueHead *hptr, CmdQueueEntry *eptr);

void shallow_destroy_cmd_queue(CmdQueueHead *hptr);

void destroy_cmd_queue(CmdQueueHead *hptr);

struct command ** cmd_queue_to_array(CmdQueueHead *hptr);

/*
 *	Other function declarations
 */

void exec_cmds(struct command **cmdv);

int general_executioner(char *cmd, char **args);

int cd_executioner(char *cmd, char **args);

int exit_executioner(char *cmd, char **args);

void destroy_cmd(struct command *cmdp);

/*
 *	Utility function declarations
 */

char * str_cat(char *s1, char *s2);

char * get_prompt();

/*
 *	Global variable definitions
 */

int ret_val;

int lineno;

#endif /* _MYSHELL_H */