OUT = mysh
OBJ_FILES = mysh.o execution.o parser_queues.o util.o command.o
CFLAGS = -Wall

ifeq (`uname`,Darwin)
	FLEX = -ll
else
	FLEX = -lfl
endif

mysh:	$(OBJ_FILES) parser.tab.c lex.yy.c
		cc $(CFLAGS) -o $(OUT) $(OBJ_FILES) parser.tab.c lex.yy.c $(FLEX) -lreadline

parser.tab.c:	parser.y
				bison -d parser.y

lex.yy.c:	scanner.l
			flex --header-file=lex.yy.h scanner.l

execution.o:	execution.c command.h parser_queues.h mysh.h lex.yy.h parser.tab.h
				cc $(CFLAGS) -c execution.c

parser_queues.o:	parser_queues.c command.h
					cc $(CFLAGS) -c parser_queues.c

util.o:		util.c mysh.h
			cc $(CFLAGS) -c util.c

command.o:	command.c command.h
			cc $(CFLAGS) -c command.c

mysh.o:		mysh.c parser.tab.h lex.yy.h mysh.h
			cc $(CFLAGS) -c mysh.c

clean:
		rm -f mysh *.o