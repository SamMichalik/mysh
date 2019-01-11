%s INTERN

%{
#include "parser.tab.h"
#include "mysh.h"
#include <stdio.h>

%}

%%

<INTERN>(cd|\'cd\'|\"cd\") 			{ BEGIN INITIAL; yylval.int_cmd = CD; return INTERNAL; }
<INTERN>(exit|\'exit\'|\"exit\") 	{ BEGIN INITIAL; yylval.int_cmd = EXIT; return INTERNAL; }
^(cd|\'cd\'|\"cd\") 				{ yylval.int_cmd = CD; return INTERNAL; }
^(exit|\'exit\'|\"exit\") 			{ yylval.int_cmd = EXIT; return INTERNAL; }
";"/[ \t]*(cd|\'cd\'|\"cd\"|exit|\'exit\'|\"exit\")    { BEGIN INTERN; return SEMICOLON; }
"|"				{ fprintf(stderr, "(defualt rule matched) "); return PIPE; }
">"				{ fprintf(stderr, "(defualt rule matched) "); return RREDIR; }
"<"				{ fprintf(stderr, "(defualt rule matched) "); return LREDIR; }
";"             { return SEMICOLON; }
"\n"            { return EOL; }
\".*\"          { yylval.charptr = yytext; return WORD; }
\'.*\'          { yylval.charptr = yytext; return WORD; }
[ \t]+          { /* ignore whitespaces */}
#[^\n]*         { /* ignore comments */ }
[^ \t\n#;]+     { yylval.charptr = yytext; return WORD; }

%%