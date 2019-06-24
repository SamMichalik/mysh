#ifndef _PARSER_QUEUES_H
#define	_PARSER_QUEUES_H

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

STAILQ_HEAD(pipeline_queue_head, pipeline_queue_entry);

struct pipeline_queue_entry {
    struct cmd_queue_head *pipeptr;
    STAILQ_ENTRY(pipeline_queue_entry) entries;
};

struct command {
	char *name;
	char **args;
    char *ldir;
    char *rdir;
    char *rrdir;
    int cmd_type;
	/* int (*executioner)(char *name, char **args); */
    /* int (*executioner)(struct command *cmdptr, char **args); */
};

/*
 *	Typedefs
 */

typedef struct string_queue_head StringQueueHead;

typedef struct string_queue_entry StringQueueEntry;

typedef struct cmd_queue_head CmdQueueHead;

typedef struct cmd_queue_entry CmdQueueEntry;

typedef struct pipeline_queue_head PipelineQueueHead;

typedef struct pipeline_queue_entry PipelineQueueEntry;

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

void destroy_cmd(struct command *cmdp);

int get_queue_len(CmdQueueHead *cqhptr);

/*
 *  Pipeline queue function declarations
 */

PipelineQueueHead * initialize_pipeline_queue(PipelineQueueEntry *e);

PipelineQueueEntry * create_pipeline_queue_entry(CmdQueueHead * cmdqueue);

void insert_pipeline_queue(PipelineQueueHead * hptr, PipelineQueueEntry * eptr);

#endif /* _PARSER_QUEUES_H */