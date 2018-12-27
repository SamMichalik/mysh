mysh:	main.c mysh.y mysh.l mysh.c
		bison -d mysh.y
		flex --header-file=lex.yy.h mysh.l
		cc -o $@ main.c mysh.c mysh.tab.c lex.yy.c -lfl -lreadline 

debug:	main.c mysh.y mysh.l mysh.c
		bison -d mysh.y
		flex --header-file=lex.yy.h mysh.l
		cc -g -o mysh.debug main.c mysh.c mysh.tab.c lex.yy.c -lfl -lreadline 