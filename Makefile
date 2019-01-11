mysh:	main.c parser.y scanner.l mysh.c
		bison -d parser.y
		flex --header-file=lex.yy.h scanner.l
		cc -o $@ main.c mysh.c parser.tab.c lex.yy.c -lfl -lreadline

debug:	main.c parser.y scanner.l mysh.c
		bison -d parser.y
		flex --header-file=lex.yy.h scanner.l
		cc -g -o mysh.debug main.c mysh.c parser.tab.c lex.yy.c -lfl -lreadline