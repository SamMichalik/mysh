#ifndef _MYSH_H
#define	_MYSH_H

#include "parser_queues.h"

#define	BUFFSIZE    1000
#define	LINELIMIT	32000

/*
 *	execution.c - Execution function declarations
 */

void exec_cmds(struct command **cmdv);

int general_executioner(char *cmd, char **args);

int cd_executioner(char *cmd, char **args);

int exit_executioner(char *cmd, char **args);

void execute_script(char *scrpath);

void execute_line(char *line);

/*
 *	util.c - Utility function declarations
 */

char * str_cat(char *s1, char *s2);

char * get_prompt();

char * buf_cpy(char *buf, int len, char *line);

int check_line_length(char *line);

/*
 *	Global variable definitions
 */

int ret_val;

int lineno;

#endif /* _MYSH_H */