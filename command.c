#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>

#include "command.h"

typedef struct command Cmd;

void
set_rdir(Cmd *cmdptr, char *redir)
{
	if (cmdptr->rrdir) {
		free(cmdptr->rrdir);
		cmdptr->rrdir = NULL;
	}
	if (cmdptr->rdir) {
		free(cmdptr->rdir);
		cmdptr->rdir = NULL;
	}
	char *s = strdup(redir);
	if (!s)
		err(1, "strdup");

	cmdptr->rdir = s;
}

void
set_rrdir(Cmd *cmdptr, char *redir)
{
	if (cmdptr->rrdir) {
		free(cmdptr->rrdir);
		cmdptr->rrdir = NULL;
	}
	if (cmdptr->rdir) {
		free(cmdptr->rdir);
		cmdptr->rdir = NULL;
	}
	char *s = strdup(redir);
	if (!s)
		err(1, "strdup");

	cmdptr->rrdir = s;
}

void
set_ldir(Cmd *cmdptr, char *redir)
{
	if (cmdptr->ldir) {
		free(cmdptr->ldir);
	}
	char *s = strdup(redir);
	if (!s)
		err(1, "strdup");

	cmdptr->ldir = s;
}

void
init_cmd(Cmd *cmdptr, char *name, int cmd_type)
{
	if (cmdptr) {
		cmdptr->name = strdup(name);
		if (!cmdptr->name)
			err(1, "strdup");
		cmdptr->cmd_type = cmd_type;
		cmdptr->ldir = NULL;
		cmdptr->rdir = NULL;
		cmdptr->rrdir = NULL;
		cmdptr->args = NULL;
	}
}