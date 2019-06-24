O_DEBUG = mysh.debug
C_FILES = execution.c parser_queues.c util.c command.c

ifeq (`uname`,Darwin)
	FLEX = -ll
else
	FLEX = -lfl
endif

mysh:	mysh.c parser.y scanner.l $(C_FILES)
		bison -d parser.y
		flex --header-file=lex.yy.h scanner.l
		cc -o $@ mysh.c $(C_FILES) parser.tab.c lex.yy.c $(FLEX) -lreadline

debug:	mysh.c parser.y scanner.l $(C_FILES)
		bison -d parser.y
		flex --header-file=lex.yy.h scanner.l
		cc -g -o $(O_DEBUG) mysh.c $(C_FILES) parser.tab.c lex.yy.c $(FLEX) -lreadline