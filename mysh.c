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

#include "parser.tab.h"
#include "lex.yy.h"
#include "mysh.h"

extern int ret_val;
extern int lineno;
extern int optind;
extern char *optarg;

void sig_handler(int sig);

int readline_line_reset(void);

int interactive_mode(void);

void usage(void);

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
				usage();
				exit(EXIT_FAILURE);
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
	if (sigemptyset(&sigs) == -1)
		err(1, "sigemptyset");
	if (sigaddset(&sigs, SIGINT) == -1)
		err(1, "sigaddset");
	if (sigprocmask(SIG_SETMASK, &sigs, &osigs) == -1)
		err(1, "sigprocmask");

	char *line = (char *)NULL;

	/* read lines from user input */
	do {
		if (line != NULL) {
			free(line);
		}

		char *prompt = get_prompt();
		if (sigprocmask(SIG_UNBLOCK, &sigs, NULL) == -1)
			err(1, "sigprocmask");
		line = readline(prompt);
		if (sigprocmask(SIG_SETMASK, &sigs, NULL) == -1)
			err(1, "sigprocmask");
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
	if (sigprocmask(SIG_UNBLOCK, &osigs, NULL) == -1)
		err(1, "sigprocmask");

	return (ret_val);
}

void
usage()
{
	fprintf(stderr, "%-8s mysh \n", "usage:");
	fprintf(stderr, "%-8s mysh -c command\n", "");
	fprintf(stderr, "%-8s mysh script ...\n", "");
}