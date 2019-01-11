#define	_XOPEN_SOURCE	700

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <err.h>
#include <signal.h>

#include "parser_queues.h"
#include "mysh.h"

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

	return cptrptr;
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
shallow_destroy_cmd_queue(CmdQueueHead *hptr)
{
	CmdQueueEntry *e1, *e2;

	e1 = STAILQ_FIRST(hptr);

	while (e1 != NULL) {
		e2 = STAILQ_NEXT(e1, entries);
		free(e1);
		e1 = e2;
	}

	STAILQ_INIT(hptr);
	free(hptr);
}

void destroy_cmd_queue(CmdQueueHead *hptr)
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

struct command **
cmd_queue_to_array(CmdQueueHead *hptr)
{
	struct command **cmdptrptr;
	int len = 1;
	int offs = 0;
	CmdQueueEntry *eptr;

	STAILQ_FOREACH(eptr, hptr, entries) { len++; }
	
	cmdptrptr = malloc(len * sizeof (struct command *));
	if (!cmdptrptr) {
		err(1, "malloc");
	}

	STAILQ_FOREACH(eptr, hptr, entries) {
		*(cmdptrptr + offs) = eptr->cmdptr;
		offs++;
	}
	*(cmdptrptr + offs) = NULL;

	return (cmdptrptr);
}

void
destroy_cmd(struct command *cmdp)
{
	free(cmdp->name);
	
	char **argv = cmdp->args;
	if (cmdp->args != NULL) {
		while(*argv != NULL) {
			free(*argv);
			argv++;
		}
	}

	free(cmdp->args);
}