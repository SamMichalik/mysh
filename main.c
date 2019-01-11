#define	_XOPEN_SOURCE   700

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <libgen.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <limits.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "mysh.tab.h"
#include "lex.yy.h"
#include "mysh.h"

#define	BUFFSIZE    1000
#define LINELIMIT	32000

extern int ret_val;
extern int lineno;
extern int optind;
extern char *optarg;

void sig_handler(int sig);

int readline_line_reset(void);

int interactive_mode(void);

void execute_script(char *scrpath);

char * buf_cpy(char *buf, int len, char *line);

void execute_line(char *line);

int check_line_length(char *line);

int
main(int argc, char **argv)
{
	/* set SIGINT handler */
	struct sigaction act = { 0 };
	act.sa_handler = sig_handler;
	if (sigaction(SIGINT, &act, NULL) == -1)
		err(1, "sigaction");

	/* set readline's signal handler */
	rl_signal_event_hook = readline_line_reset;

	int opt;
	char *bname;
	while ((opt = getopt(argc, argv, "c:")) != -1) {

		switch (opt) {
			case 'c': {

				/*
				 * Artifically append a newline to simplify bison's
				 * parsing. ( as an alternative to complicating the 
				 * grammar to handle this situation )
				 */
				char *nl = str_cat(optarg, "\n");

				if (check_line_length(nl)) {
					execute_line(nl);
				}

				free(nl);

				return (ret_val);
			}
			case '?':
				/* Todo: Improve usage message */
				bname = basename(argv[0]);
				fprintf(stderr, "usage: %s \n", bname);
				exit(EXIT_FAILURE);
				break;
		}
	}

	if (argv[optind] == NULL) {
		/* interactive mode */
		int rv = interactive_mode();
		return (rv);

	} else {
		/* non-interactive mode */
		argv = &argv[optind];

		while (*argv != NULL) {
			execute_script(*argv);
			argv++;
		}
	}

	return (0);
}

void
sig_handler(int sig)
{
}

int
readline_line_reset(void)
{
	printf("\n");
	rl_on_new_line();
	rl_replace_line("", 0);
	rl_redisplay();

	return (0);
}

int
interactive_mode(void)
{
	sigset_t sigs, osigs;
	sigemptyset(&sigs);
	sigaddset(&sigs, SIGINT);
	sigprocmask(SIG_SETMASK, &sigs, &osigs);

	char *line = (char *)NULL;

	/* read lines from user input */
	do {
		if (line != NULL) {
			free(line);
		}

		char *prompt = get_prompt();
		sigprocmask(SIG_UNBLOCK, &sigs, NULL);
		line = readline(prompt);
		sigprocmask(SIG_SETMASK, &sigs, NULL);
		free(prompt);

		/* parse the line and execute the commands sequentially */
		if (line != NULL) {
			add_history(line);

			/*
			 * Readline's stripping of the newline character 
			 * complicates bison's parsing. Rather than
			 * changing the grammar we just append the newline
			 * artificially.
			 */
			char *nl = str_cat(line, "\n");
			free(line);
			line = nl;

			if (check_line_length(line)) {
				execute_line(line);
			}
		}
	} while (line != NULL);

	printf("\n");
	sigprocmask(SIG_UNBLOCK, &osigs, NULL);

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

		if ( r == 0 && line != NULL) {
			int len = strlen(line);
			if (line[len - 1] != '\n') {
				char *nl = str_cat(line, "\n");
				free(line);
				line = nl;
			}

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

char *
buf_cpy(char *buf, int len, char *line)
{
	char *sub_line = malloc((len + 1) * sizeof (char));
	for (int i = 0; i < len; i++) {
		*(sub_line + i) = *(buf + i);
	}
	*(sub_line + len) = '\0';

	if (!line) {
		line = sub_line;
	} else {
		char *new_line = str_cat(line, sub_line);
		free(line);
		free(sub_line);
		line = new_line;
	}
	return (line);
}

void
execute_line(char *line)
{
	if (line) {
		yy_scan_string(line);
		yyparse();
	}
}

int
check_line_length(char *line)
{
	if (strlen(line) > LINELIMIT) {
		ret_val = 1;
		fprintf(stderr, "mysh: maximum line length exceeded\n");
		return (0);
	}
	return (1);
}