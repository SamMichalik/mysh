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

extern int ret_val;
extern int lineno;
extern int optind;
extern char *optarg;

void sig_handler(int sig);

int readline_line_reset(void);

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
	while ((opt = getopt(argc, argv, "c:")) != -1) {

		switch (opt) {
			case 'c': {

				/*
				 * Artifically append a newline to simplify bison's parsing.
				 * ( as an alternative to complicating the grammar to handle
				 *  this situation )
				 */
				char *nl = str_cat(optarg, "\n");

				yy_scan_string(nl);
				yyparse();

				free(nl);

				return (ret_val);
			}
			case '?':
				/* Todo: Improve usage message */
				fprintf(stderr, "usage: %s \n", basename(argv[0]));
				exit(EXIT_FAILURE);
				break;
		}
	}

	if (argv[optind] == NULL) {
		/*
		 * Interactive mode
		 */

		sigset_t sigs, osigs;
		sigemptyset(&sigs);
		sigaddset(&sigs, SIGINT);
		sigprocmask(SIG_SETMASK, &sigs, &osigs);

		char *line = (char *)NULL;

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
				 * Readline's stripping of the newline character complicates
				 * bison's parsing. Rather than changing the grammar we just
				 * append the newline artificially.
				 */
				char *nl = str_cat(line, "\n");
				free(line);
				line = nl;

				yy_scan_string(line);
				yyparse();
			}
		} while (line != NULL);

		printf("\n");
		sigprocmask(SIG_UNBLOCK, &osigs, NULL);

		return (ret_val);
		
	} else {
		/*
		 * Non-interactive mode
		 */

		argv = &argv[optind];

		char *fpath;
		while (*argv != NULL) {
			fpath = realpath(*argv, NULL);

			/*
			 * Reading input from files could be simplified
			 * to a few lines if fopen was allowed.
			 */
			int fd = open(fpath, O_RDONLY);
			if (fd == -1) {
				err(1, "open");
			} else {
				char buf[BUFFSIZE];
				char *line = (char *)NULL;
				int r;

				lineno = 0;
				while ((r = read(fd, buf, BUFFSIZE)) != -1) {
					int pos = 0;
					int old_pos = -1;

					while (pos < r) {
						/* on newline encounter execute the built line */
						if (buf[pos] == 10) {
							int len = (pos - old_pos);
							char *sub_line = malloc((len + 1) * sizeof (char));

							for (int i = 0; i < len; i++) {
								*(sub_line + i) = buf[old_pos + i + 1];
							}
							*(sub_line + len) = '\0';
							if (line == NULL) {
								line = sub_line;
							} else {
								char *new_line = str_cat(line, sub_line);
								free(line);
								free(sub_line);
								line = new_line;
							}

							lineno++;
							yy_scan_string(line);
							yyparse();
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

					if (r == 0) {
						break;
					}
				}

				if (r == -1) {
					err(1, "read");
				}
			}

			if (close(fd) == -1) {
				err(1, "close");
			}
			free(fpath);
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
}