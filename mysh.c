#define	_XOPEN_SOURCE	700

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <err.h>
#include <signal.h>

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

/*
 *	Other function definitions
 */

void
exec_cmds(struct command **cmdv)
{
	if (cmdv != NULL) {
		while (*cmdv != NULL) {
			/*
			 * Count and relocate args so that cmd name is 1st.
			 * This is provisional.
			 */
			int argc = 0;
			char **args;
			struct command *cmdptr = *cmdv;

			if (cmdptr->args != NULL) {
				args = cmdptr->args;
				while (*(args + argc) != NULL) { argc++; }
			}
			char **argv = malloc((argc + 2) * sizeof (char *)); /* cmd + argv + NULL */
			if (!argv) {
				err(1, "malloc");
			}
			*argv = cmdptr->name;
			for (int i = 0; i < argc; i++) {
				*(argv + i + 1) = *(args + i);
			}
			*(argv + argc + 1) = NULL;

			/* execute the command */
			ret_val = cmdptr->executioner(cmdptr->name, argv);

			/* release memory referenced by command contents */
			destroy_cmd(cmdptr);
			/* release the command structure */
			free(cmdptr);
			/* release the argument vector */
			free(argv);

			cmdv++;
		}   
	}
}

int
general_executioner(char *cmd, char **args)
{
	int pid, ret_val, i;

	sigset_t sigs;
	if (sigemptyset(&sigs) == -1)
		err(1, "sigemptyset");
	if (sigaddset(&sigs, SIGINT) == -1)
		err(1, "sigaddset");


	if (sigprocmask(SIG_UNBLOCK, &sigs, NULL) == -1)
		err(1, "sigprocmask");
	switch(pid = fork()) {
		case -1:
			err(1, "fork");
			break;

		case 0:
			execvp(cmd, args);
			fprintf(stderr, "mysh: %s: No such file or directory\n", cmd);
			exit(127);
			break;

		default:
			if (sigprocmask(SIG_SETMASK, &sigs, NULL) == -1)
				err(1, "sigprocmask");
			if (wait(&i) == -1)
				err(1, "wait");
			if (WIFSIGNALED(i) != 0) {
				fprintf(stderr, "Killed by signal %d\n", WTERMSIG(i));
				ret_val = 128 + WTERMSIG(i);
			} else {
				ret_val = WEXITSTATUS(i);
			}
			break;
	}
	return (ret_val);
}

int
cd_executioner(char *cmd, char **args)
{
	int ret_val = 0;

	char *cwd = getcwd(NULL, 0);
	if (!cwd) {
		err(1, "getcwd");
	}

	if (*(args + 1) == NULL) {

		char *home = getenv("HOME");
		if (!home) {
			fprintf(stderr, "mysh: cd: HOME not set\n");
			ret_val = 1;
			return ret_val;
		}

		if (chdir(home) == -1) {
			err(1, "chdir");
		} 
		
		char *s1 = "PWD=";
		char *s2 = "OLDPWD=";

		if (setenv("OLDPWD", cwd, 1) == -1)
			err(1, "setenv");

		if (setenv("PWD", home, 1) == -1)
			err(1, "setenv");

	} else if (*(args + 2) == NULL) {

		char *nwd;

		if (strcmp(*(args + 1), "-") == 0 ) {
			nwd = str_dup(getenv("OLDPWD"));
			if (!nwd) {
				fprintf(stderr, "mysh: cd: OLDPWD not set\n");
				ret_val = 1;
				return ret_val;
			}
		} else {
			/* Todo: handle paths starting with ~/ */
			nwd = realpath(*(args + 1), NULL);
			if (!nwd)
				err(1, "realpath");
		}

		if (chdir(nwd) == -1) {
			fprintf(stderr, "mysh: cd: %s: No such file or directory\n", *(args + 1));
			ret_val = 1;
		} else {
			char *s1 = "PWD=";
			char *s2 = "OLDPWD=";

			if (setenv("OLDPWD", cwd, 1) == -1)
				err(1, "setenv");

			if (setenv("PWD", nwd, 1) == -1)
				err(1, "setenv");
		}

		free(nwd);
	} else {
		fprintf(stderr, "mysh: cd: too many arguments\n");
		ret_val = 1;
	}

	free(cwd);
	return (ret_val);
}

int
exit_executioner(char *cmd, char **args)
{
	exit(ret_val);
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

/*
 *	Utility function definitions
 */

char *
str_dup(char *s)
{
	if (!s) {
		return (char *)NULL;
	}	

	int len = 0;
	char *ns;

	len = strlen(s) + 1;
	ns = malloc(len * sizeof (char));
	if (!ns) {
		err(1, "malloc");
	}

	for (int j = 0; j < len; j++) {
		*(ns + j) = *(s + j);
	}

	return ns;
}

char *
str_cat(char * s1, char * s2)
{
	int l1 = strlen(s1);
	int l2 = strlen(s2);
	char *s = malloc((l1 + l2 + 1) * sizeof (char));
	if (!s) {
		err(1, "malloc");
	}
	int offs = 0;

	while (*s1 != '\0') {
		*(s + offs) = *s1;
		s1++;
		offs++;
	}
	while (*s2 != '\0') {
		*(s + offs) = *s2;
		s2++;
		offs++;
	}
	*(s + offs) = '\0';

	return (s);
}


char *
get_prompt()
{
	char *cwd = getcwd(NULL, 0);
	if (!cwd) {
		err(1, "getcwd");
	}

	char *s = "mysh:";
	char *s2 = "$ ";
	s = str_cat(s, cwd);
	s2 = str_cat(s, s2);
	free(s);
	free(cwd);
	return (s2);
}