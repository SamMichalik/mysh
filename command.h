#ifndef _COMMAND_H
#define	_COMMAND_H

enum cmd_type {
	CD,
	EXIT,
	GENERAL
};

struct command {
	char *name;
	char **args;
	char *ldir;
	char *rdir;
	char *rrdir;
	int cmd_type;
};

typedef struct command Cmd;

void set_rdir(Cmd *cmdptr, char *redir);

void set_rrdir(Cmd *cmdptr, char *redir);

void set_ldir(Cmd *cmdptr, char *redir);

void init_cmd(Cmd *cmdptr, char *name, int cmd_type);

#endif /* _COMMAND_H */