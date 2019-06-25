OUT = mysh
C_FILES = execution.c parser_queues.c util.c command.c

ifeq (`uname`,Darwin)
	FLEX = -ll
else
	FLEX = -lfl
endif

mysh:	mysh.c parser.y scanner.l $(C_FILES)
		bison -d parser.y
		flex --header-file=lex.yy.h scanner.l
		cc $(CFLAGS) -o $(OUT) mysh.c $(C_FILES) parser.tab.c lex.yy.c $(FLEX) -lreadline

clean:
		rm -f mysh mysh.debug