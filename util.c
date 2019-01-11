#include <stdio.h>
#include <err.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "mysh.h"

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

char *
buf_cpy(char *buf, int len, char *line)
{
	char *sub_line = malloc((len + 1) * sizeof (char));
	if (!sub_line) {
		err(1, "malloc");
	}

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