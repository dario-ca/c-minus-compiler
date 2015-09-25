/**********************/
/* C header files */
/**********************/

%{
#include "globals.h"
#include "ast.h"
#include "cmparser.tab.h"

%}

%option yylineno


/**********************/
/* start your regular definitions  here */
/**********************/

LINENO      \n
SPACE       [ \t\r]
MULTICOMMENT (\/\*)([^*]|\*+[^\*\/])*(\*+\/)
LINECOMMENT (\/\/).*
    
ELSE        else
IF          if
RETURN      return
VOID        void
INT         int
WHILE       while
ID          [_$a-zA-Z][_$a-zA-Z0-9]*
NUMBER		(0|[1-9][0-9]*)
NUM_ERROR   0+[0-9]+
PLUS        \+
MINUS       \-
MULT        \*
DIV         \/
LT          <
LE          <=
GT          >
GE          >=
EQ          ==
NE          !=
ASSIGN      =
SEMI        ;
COMMA       ,
LPAREN      \(
RPAREN      \)
LSQ         \[
RSQ         \]
LBRACE      \{
RBRACE      \}

ERROR       (auto|break|case|char|const|continue|default|do|double|for|goto|long|short|register|switch|byte|float|~|\?|\:|\+\+|\-\-|!|\||\^|<<|>>|>>>|\+=|\-=|\*=|\/=|\^=|\%=|<<=|>>=|&=|\|=|>>>=)

OTHER_ERROR .

/* start your token specifications here */
/* Token names must come from cmparser.tab.h */

%%

{LINENO} {}
{SPACE} {}

{MULTICOMMENT} {}

{LINECOMMENT} {}

{ERROR} { return TOK_ERROR; }

{ELSE} { return TOK_ELSE; }
{IF} { return TOK_IF; }
{RETURN} { return TOK_RETURN; }
{VOID} { return TOK_VOID; }
{INT} { return TOK_INT; }
{WHILE} { return TOK_WHILE; }
{NUMBER} { yylval.iVal = atoi(yytext); return TOK_NUM; }
{ID} { yylval.cVal = strdup(yytext); return TOK_ID; }
{NUM_ERROR} { return TOK_ERROR; }
{PLUS} { return TOK_PLUS; }
{MINUS} { return TOK_MINUS; }
{MULT} { return TOK_MULT; }
{DIV} { return TOK_DIV; }
{LT} { return TOK_LT; }
{LE} { return TOK_LE; }
{GT} { return TOK_GT; }
{GE} { return TOK_GE; }
{EQ} { return TOK_EQ; }
{NE} { return TOK_NE; }
{ASSIGN} { return TOK_ASSIGN; }
{SEMI} { return TOK_SEMI; }
{COMMA} { return TOK_COMMA; }
{LPAREN} { return TOK_LPAREN; }
{RPAREN} { return TOK_RPAREN; }
{LSQ} { return TOK_LSQ; }
{RSQ} { return TOK_RSQ; }
{LBRACE} { return TOK_LBRACE; }
{RBRACE} { return TOK_RBRACE; }


{OTHER_ERROR} { return TOK_ERROR; }



%%
/**********************/
/* C support functions */
/**********************/


int initLex(int argc, char **argv){

    
   if ( argc > 1 )
       yyin = fopen( argv[1], "r" );
   else
    yyin = stdin;
    
    return 0;
} 
