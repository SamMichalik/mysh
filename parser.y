%{
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <sys/queue.h>

#include "lex.yy.h"
#include "parser_queues.h"
#include "mysh.h"
#include "command.h"

void yyerror(char *s);

extern char *yytext;
extern int lineno;

%}

%union{
    struct pipeline_queue_head *pqh_ptr;    /* alias PipelineQueueHead */
    struct string_queue_head *sqh_ptr;     /* alias StringQueueHead */
    struct cmd_queue_head *cqh_ptr;        /* alias CmdQueueHead */
    struct command *cmdptr;
    struct command **cmdptrptr;
    char *charptr;
    int int_cmd;
}

%token SEMICOLON
%token <charptr> WORD
%token EOL
%token <int_cmd> INTERNAL
%token PIPE
%token LREDIR
%token RREDIR
%token RREDIR_APPEND

%type <sqh_ptr> args_seq
%type <pqh_ptr> cmd_seq
%type <cqh_ptr> pipe_cmd
%type <cmdptr> base_cmd
%type <cmdptr> redir_cmd
%type <charptr> cmd_name
%type <pqh_ptr> line

%%

start:	line 
	{
        if ($1 != NULL) {
            exec_cmds($1);
            destroy_pipeline_queue($1);    
        }
	}

line:   EOL
	{ 
		$$ = NULL; 
	}
	|	cmd_seq SEMICOLON EOL
	{
        $$ = $1;
	}
	|	cmd_seq EOL
	{
        $$ = $1;
	}

cmd_seq: pipe_cmd
	{
        PipelineQueueEntry *eptr = create_pipeline_queue_entry($1);
        $$ = initialize_pipeline_queue(eptr);
	}
	| cmd_seq SEMICOLON pipe_cmd
	{
        PipelineQueueEntry *eptr = create_pipeline_queue_entry($3);
        insert_pipeline_queue($1, eptr);
        $$ = $1;
	}

pipe_cmd: redir_cmd
    {
        CmdQueueEntry *eptr = create_cmd_queue_entry($1);
		$$ = initialize_cmd_queue(eptr);
    }
    | pipe_cmd PIPE redir_cmd
    {
        CmdQueueEntry *eptr = create_cmd_queue_entry($3);
		insert_cmd_queue($1, eptr);
		$$ = $1;
    }

redir_cmd: base_cmd
    {
        $$ = $1;
    }
    | redir_cmd RREDIR WORD
    {
        $$ = $1;
        set_rdir($$, $3);
    }
    | redir_cmd RREDIR_APPEND WORD
    {
        $$ = $1;
        set_rrdir($$, $3);
    }
    | redir_cmd LREDIR WORD
    {
        $$ = $1;
        set_ldir($$, $3);
    }


base_cmd: cmd_name
	{
		struct command *cmdptr = malloc(sizeof(struct command));
		if (!cmdptr)
			err(1, "malloc");
        init_cmd(cmdptr, $1, GENERAL);
        free($1);
		$$ = cmdptr;
	}
	| cmd_name args_seq
	{
		struct command *cmdptr = malloc(sizeof(struct command));
		if (!cmdptr)
			err(1, "malloc");
        init_cmd(cmdptr, $1, GENERAL);
        cmdptr->args = string_queue_to_array($2);
		shallow_destroy_string_queue($2);
        free($1);
		$$ = cmdptr;
	}
	| INTERNAL
	{
		struct command *cmdptr = malloc(sizeof(struct command));
		if (!cmdptr)
			err(1, "malloc");
		switch ($1) {
			case CD:
                init_cmd(cmdptr, "cd", CD);
				break;
			case EXIT:
				init_cmd(cmdptr, "exit", EXIT);
				break;
		}
		$$ = cmdptr;
	}
	| INTERNAL args_seq
	{
		struct command *cmdptr = malloc(sizeof(struct command));
		if (!cmdptr)
			err(1, "malloc");

		switch ($1) {
			case CD:
				init_cmd(cmdptr, "cd", CD);
				break;
			case EXIT:
				init_cmd(cmdptr, "exit", EXIT);
				break;
		}
        cmdptr->args = string_queue_to_array($2);
		shallow_destroy_string_queue($2);
		$$ = cmdptr;
	}

cmd_name: WORD 
	{ 
		char *s = strdup($1);
		if (!s)
			err(1, "strdup");
		$$ = s; 
	}

args_seq: WORD  
	{ 
		char *s = strdup($1);
		if (!s)
			err(1, "strdup");
		StringQueueEntry *qeptr = create_string_queue_entry(s);
		$$ = initialize_string_queue(qeptr); 
	}
	| args_seq WORD 
	{ 
		char *s = strdup($2);
		if (!s)
			err(1, "strdup");
		StringQueueEntry *qeptr = create_string_queue_entry(s);
		insert_string_queue($1, qeptr);
		$$ = $1; 
	}

%%

void
yyerror(char *s)
{
	fprintf(stderr, "error:%d: %s near unexpected token '%s'\n", lineno, s, yytext);
	ret_val = 254;
}