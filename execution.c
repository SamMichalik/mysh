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
#include "command.h"

char * prep_line(char *line);

int cd_home(char *cwd);

int ch_dir(char **args);

void exec_pipeline(CmdQueueHead *cmdqhptr);

int exec_piped_cmd(struct command *cmdptr, char **argv);

int exec_nonpiped_cmd(struct command *cmdptr, char **argv);

char **
get_execvp_args(struct command *cmdptr)
{
    if (cmdptr != NULL) {
        char **args;
        char **argv;
        int argc = 0;

        /* compute the number of arguments */
        if (cmdptr->args != NULL) {
            args = cmdptr->args;
            while (*(args + argc) != NULL) { argc++; }
        }
        /* allocate space for the argument vector */
        argv = malloc((argc + 2) * sizeof (char *)); /* cmd_name + argv + NULL */
        if (!argv)
            err(1, "malloc");
        /* set the values of the array */
        if (!cmdptr->name)
            return NULL;
        
        *argv = cmdptr->name;
		for (int i = 0; i < argc; i++) {
			*(argv + i + 1) = *(args + i);
		}
		*(argv + argc + 1) = NULL;
        
        return argv;
    }
    return NULL;
}

void
redirect(struct command *cmdptr)
{
    if (cmdptr->ldir != NULL) {
        if (close(0) < 0) {
            err(1, "close");
        }
        if (open(cmdptr->ldir, O_RDONLY) < 0) {
            err(1, "open");
        }
    }

    if (cmdptr->rdir != NULL) {
        if (close(1) < 0) {
            err(1, "close");
        }
        if (open(cmdptr->rdir, O_WRONLY | O_CREAT | O_TRUNC, 0666) < 0) {
            err(1, "open");
        }
    }
    else if (cmdptr->rrdir != NULL) {
        if (close(1) < 0) {
            err(1, "close");
        }
        if (open(cmdptr->rrdir, O_WRONLY | O_CREAT | O_APPEND, 0666) < 0) {
            err(1, "open");
        }
    }
}

void
restore_std_io(int stdin_fd, int stdout_fd)
{
    if (close(0) == -1)
        err(1, "close");
    if (dup(stdin_fd) == -1)
        err(1, "dup");
    if (close(stdin_fd) == -1)
        err(1, "close");

    if (close(1) == -1)
        err(1, "close");
    if (dup(stdout_fd) == -1)
        err(1, "dup");
    if (close(stdout_fd) == -1)
        err(1, "close");
}

void
exec_cmds(PipelineQueueHead *pqhptr)
{
    if (pqhptr != NULL) {
        PipelineQueueEntry *eptr;

        STAILQ_FOREACH(eptr, pqhptr, entries) {
            /* each entry is a pipeline of commands */
            CmdQueueHead *cqhptr = eptr->pipeptr;
            exec_pipeline(cqhptr);
        }
    }
}

void
exec_pipeline(CmdQueueHead *cqhptr)
{
    if (cqhptr != NULL) {
        CmdQueueEntry *eptr;
        int pipeline_len = get_queue_len(cqhptr);
        int cmd_pids[pipeline_len];
        int i = -1;
        int stat_loc;
        
        /* store std in & out for later as it might be replaced by pipe file descs. */
        int stdin_dup_fd = dup(0);
        int stdout_dup_fd = dup(1);
        
        if (stdin_dup_fd == -1)
            err(1, "dup");
        if (stdout_dup_fd == -1)
            err(1, "dup");
        

        STAILQ_FOREACH(eptr, cqhptr, entries) {
            struct command *cmdptr = eptr->cmdptr;

            char **argv = get_execvp_args(cmdptr);
            i++;

            if (cmdptr->cmd_type == CD) {
                cmd_pids[i] = -1;
                ch_dir(argv);
            }
            else if (cmdptr->cmd_type == EXIT) {
                cmd_pids[i] = -1;
                exit(ret_val);
            }
            else {
                int pid;

                sigset_t sigs;
	            if (sigemptyset(&sigs) == -1)
		            err(1, "sigemptyset");
	            if (sigaddset(&sigs, SIGINT) == -1)
		            err(1, "sigaddset");
	            if (sigprocmask(SIG_UNBLOCK, &sigs, NULL) == -1)
		            err(1, "sigprocmask");

                if (eptr->entries.stqe_next != NULL) {
                    /* command has a succesor in the pipeline */
                    pid = exec_piped_cmd(cmdptr, argv);
                    cmd_pids[i] = pid;
                }
                else {
                    /* last command in the pipeline */
                    pid = exec_nonpiped_cmd(cmdptr, argv);
                    cmd_pids[i] = pid;
                }
                if (sigprocmask(SIG_SETMASK, &sigs, NULL) == -1)
				err(1, "sigprocmask");
            }

            free(argv);
        }

        /* here we would like to wait for all the child processes */
        for (int i = 0; i < pipeline_len; i++) {
            if (cmd_pids[i] != -1) {
                stat_loc = 0;
                if (waitpid(cmd_pids[i], &stat_loc, 0) == -1)
                    err(1, "waitpid");
                if (WIFSIGNALED(stat_loc) != 0) {
				    fprintf(stderr, "Killed by signal %d.\n", WTERMSIG(stat_loc));
				    ret_val = 128 + WTERMSIG(stat_loc);
			    } else {
				    ret_val = WEXITSTATUS(stat_loc);
			    }
            }
        }

        restore_std_io(stdin_dup_fd, stdout_dup_fd);
    }
}

int
exec_piped_cmd(struct command *cmdptr, char **argv)
{
    int pd[2];
                    
    if (pipe(pd) == -1)
        err(1, "pipe");
    int pid = fork();
    switch(pid) {
        case -1:
            err(1, "fork");
            break;
        case 0:
            /* child / producer */
            close(1);
            dup(pd[1]);
            close(pd[0]);
            close(pd[1]);
            /* execute command */
            redirect(cmdptr);
            execvp(cmdptr->name, argv);
			fprintf(stderr, "mysh: %s: No such file or directory\n", cmdptr->name);
            exit(127);
            break;
        default:
            /* parent / consumer */
            close(0);
            dup(pd[0]);
            close(pd[0]);
            close(pd[1]);
            break;
    }
    return pid;
}

int
exec_nonpiped_cmd(struct command *cmdptr, char **argv)
{
    int pid = fork();
    switch(pid) {
        case -1:
            err(1, "fork");
            break;
        case 0:
            redirect(cmdptr);
            execvp(cmdptr->name, argv);
            fprintf(stderr, "mysh: %s: No such file or directory\n", cmdptr->name);
            exit(127);
            break;
        default:
            break;
    }
    return pid;
}

int
ch_dir(char **args)
{
    int ret_val = 0;

	char *cwd = getcwd(NULL, 0);
	if (!cwd) {
		err(1, "getcwd");
	}

	if (*(args + 1) == NULL) {
		/* called without arguments - go home */
		if (cd_home(cwd) == 1) {
			return (ret_val);
		}

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
                char *nl = str_cat(line, "\n");
				execute_line(nl);
                free(nl);
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
		YY_BUFFER_STATE s = yy_scan_string(line);
		yyparse();
		yy_delete_buffer(s);
	}
}

char *
prep_line(char *line)
{
	if (line) {
		int len = strlen(line);
		if (len > 0) {
			if (line[len - 1] != '\n') {
				char *nl = str_cat(line, "\n");
				free(line);
				line = nl;
			}
		}
	}
	return (line);
}

int
cd_home(char *cwd)
{
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

	return (0);
}