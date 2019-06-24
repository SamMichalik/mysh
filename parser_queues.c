#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <err.h>

#include "parser_queues.h"
#include "mysh.h"
#include "command.h"

/*
 *	String queue function definitions
 */

StringQueueHead *
initialize_string_queue(StringQueueEntry *e)
{
	StringQueueHead *qhptr = malloc(sizeof (StringQueueHead));
	if (!qhptr) {
		err(1, "malloc");
	}
	STAILQ_INIT(qhptr);

	STAILQ_INSERT_TAIL(qhptr, e, entries);
	return (qhptr);
}

StringQueueEntry *
create_string_queue_entry(char *s)
{
	StringQueueEntry *qeptr = malloc(sizeof (StringQueueEntry));
	if (!qeptr) {
		err(1, "malloc");
	}
	qeptr->string = s;
	return (qeptr);
}

void
insert_string_queue(StringQueueHead *qhptr, StringQueueEntry *qeptr)
{
	STAILQ_INSERT_TAIL(qhptr, qeptr, entries);
}

void
shallow_destroy_string_queue(StringQueueHead *qhptr)
{
	StringQueueEntry *e1, *e2;
	e1 = STAILQ_FIRST(qhptr);
	while (e1 != NULL) {
		e2 = STAILQ_NEXT(e1, entries);
		free(e1);
		e1 = e2;
	}
	STAILQ_INIT(qhptr);
	free(qhptr);
}

char **
string_queue_to_array(StringQueueHead *qhptr)
{
	char **cptrptr;
	int len = 1;
	int offs = 0;
	StringQueueEntry *qeptr;

	STAILQ_FOREACH(qeptr, qhptr, entries) { len++; }
	cptrptr = malloc(len * sizeof (char *));
	if (!cptrptr) {
		err(1, "malloc");
	}
	STAILQ_FOREACH(qeptr, qhptr, entries) {
		*(cptrptr + offs) = qeptr->string;
		offs++;
	}
	*(cptrptr + offs) = NULL;

	return (cptrptr);
}

/*
 *	Cmd queue function definitions
 */

CmdQueueHead *
initialize_cmd_queue(CmdQueueEntry *eptr)
{
	CmdQueueHead *hptr = malloc(sizeof (CmdQueueHead));
	if (!hptr) {
		err(1, "malloc");
	}
	STAILQ_INIT(hptr);
	STAILQ_INSERT_TAIL(hptr, eptr, entries);
	return (hptr);
}

CmdQueueEntry *
create_cmd_queue_entry(struct command *cmd)
{
	CmdQueueEntry *eptr = malloc(sizeof (CmdQueueEntry));
	if (!eptr) {
		err(1, "malloc");
	}
	eptr->cmdptr = cmd;
	return (eptr);
}

void
insert_cmd_queue(CmdQueueHead *hptr, CmdQueueEntry *eptr)
{
	STAILQ_INSERT_TAIL(hptr, eptr, entries);
}

void
destroy_cmd_queue(CmdQueueHead *hptr)
{
	CmdQueueEntry *e1, *e2;

	e1 = STAILQ_FIRST(hptr);

	while (e1 != NULL) {
		e2 = STAILQ_NEXT(e1, entries);
		destroy_cmd(e1->cmdptr);
		free(e1->cmdptr);
		free(e1);
		e1 = e2;
	}

	STAILQ_INIT(hptr);
	free(hptr);
}

void
destroy_cmd(struct command *cmdp)
{
	if (cmdp->args != NULL) {
		char **argv = cmdp->args;
		while (*argv != NULL) {
			free(*argv);
			argv++;
		}
		free(cmdp->args);
	}
	if (cmdp->name) {
		free(cmdp->name);
	}
	if (cmdp->ldir) {
		free(cmdp->ldir);
	}
	if (cmdp->rdir) {
		free(cmdp->rdir);
	}
	if (cmdp->rrdir) {
		free(cmdp->rrdir);
	}
}

int
get_queue_len(CmdQueueHead *cqhptr)
{
	int len = 0;
	CmdQueueEntry *eptr;
	STAILQ_FOREACH(eptr, cqhptr, entries) { len++; }
	return (len);
}

/*
 *  Pipeline queue function definitions
 */

PipelineQueueHead *
initialize_pipeline_queue(PipelineQueueEntry *eptr)
{
	PipelineQueueHead *hptr = malloc(sizeof (PipelineQueueHead));
	if (!hptr) {
		err(1, "malloc");
	}
	STAILQ_INIT(hptr);
	STAILQ_INSERT_TAIL(hptr, eptr, entries);
	return (hptr);
}

PipelineQueueEntry *
create_pipeline_queue_entry(CmdQueueHead *cmdqueue)
{
	PipelineQueueEntry *eptr = malloc(sizeof (PipelineQueueEntry));
	if (!eptr) {
		err(1, "malloc");
	}
	eptr->pipeptr = cmdqueue;
	return (eptr);
}

void
insert_pipeline_queue(PipelineQueueHead *hptr, PipelineQueueEntry *eptr)
{
	STAILQ_INSERT_TAIL(hptr, eptr, entries);
}

void
destroy_pipeline_queue(PipelineQueueHead *hptr)
{
	PipelineQueueEntry *e1, *e2;

	e1 = STAILQ_FIRST(hptr);

	while (e1 != NULL) {
		e2 = STAILQ_NEXT(e1, entries);
		destroy_cmd_queue(e1->pipeptr);
		free(e1);
		e1 = e2;
	}

	STAILQ_INIT(hptr);
	free(hptr);
}