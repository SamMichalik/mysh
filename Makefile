mysh:	mysh.c parser.y scanner.l execution.c parser_queues.c util.c
		bison -d parser.y
		flex --header-file=lex.yy.h scanner.l
		cc -o $@ mysh.c execution.c parser_queues.c util.c parser.tab.c lex.yy.c -lfl -lreadline

debug:	mysh.c parser.y scanner.l execution.c parser_queues.c util.c
		bison -d parser.y
		flex --header-file=lex.yy.h scanner.l
		cc -g -o mysh.debug mysh.c parser_queues.c util.c parser.tab.c lex.yy.c -lfl -lreadline