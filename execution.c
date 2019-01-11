#define	_XOPEN_SOURCE	700

#include <stdio.h>
#include <signal.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#include "parser.tab.h"
#include "lex.yy.h"
#include "mysh.h"
#include "parser_queues.h"

char * prep_line(char *line);

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
	switch (pid = fork()) {
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
		/* called without arguments - go home */
		char *home = getenv("HOME");
		if (!home) {
			fprintf(stderr, "mysh: cd: HOME not set\n");
			ret_val = 1;
			return (ret_val);
		}

		if (chdir(home) == -1) {
			err(1, "chdir");
		}

		if (setenv("OLDPWD", cwd, 1) == -1)
			err(1, "setenv");

		if (setenv("PWD", home, 1) == -1)
			err(1, "setenv");

	} else if (*(args + 2) == NULL) {
		/* called with one argument */
		char *nwd;

		if (strcmp(*(args + 1), "-") == 0) {
			/* command was 'cd -' */
			if (!getenv("OLDPWD")) {
				fprintf(stderr, "mysh: cd: OLDPWD not set\n");
				ret_val = 1;
				return (ret_val);
			}
			nwd = strdup(getenv("OLDPWD"));
			if (!nwd) {
				err(1, "strdup");
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
execute_script(char *scrpath)
{
	/*
	 * After the script is executed, we want
	 * to return to the original location.
	 */
	char *cwd = getcwd(NULL, 0);
	if (!cwd) {
		err(1, "getcwd");
	}

	char *fpath = realpath(scrpath, NULL);
	if (!fpath) {
		err(1, "realpath");
	}

	int fd = open(fpath, O_RDONLY);
	if (fd == -1) {
		err(1, "open");
	} else {
		char buf[BUFFSIZE];
		char *line = (char *)NULL;
		int r;

		lineno = 0;
		while ((r = read(fd, buf, BUFFSIZE)) > 0) {
			int pos = 0;
			int old_pos = -1;

			while (pos < r) {
				/* on newline encounter execute the built line */
				if (buf[pos] == 10) {
					int len = (pos - old_pos);
					line = buf_cpy((buf + old_pos + 1), len, line);

					lineno++;
					if (check_line_length(line)) {
						execute_line(line);
					}
					free(line);
					line = (char *)NULL;
					old_pos = pos;

					/* encountered a syntax error */
					if (ret_val == 254) {
						exit(ret_val);
					}
				}

				pos++;
			}
			/* store the unread remains of the buffer */
			int len = (pos - old_pos) - 1;
			line = buf_cpy((buf + old_pos + 1), len, line);
		}

		if (r == 0 && line != NULL) {
			line = prep_line(line);

			lineno++;
			if (check_line_length(line)) {
				execute_line(line);
			}
			free(line);
			line = (char *)NULL;

			/* encountered a syntax error */
			if (ret_val == 254) {
				exit(ret_val);
			}
		} else if (r == -1) {
			err(1, "read");
		}
	}

	if (close(fd) == -1) {
		err(1, "close");
	}
	free(fpath);

	if (chdir(cwd) == -1) {
		err(1, "chdir");
	}
}

void
execute_line(char *line)
{
	if (line) {
		yy_scan_string(line);
		yyparse();
	}
}

char *
prep_line(char *line)
{
	if (line) {
		int len = strlen(line);
		if (line[len - 1] != '\n') {
			char *nl = str_cat(line, "\n");
			free(line);
			line = nl;
		}
	}
	return (line);
}