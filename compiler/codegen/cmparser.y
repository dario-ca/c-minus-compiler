%{

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ast.h"
#include "symbolTable.h"
#include "util.h"
#include "typecheck.h"

#define FIRST 0
#define SECOND 1
#define THIRD 2

/* external function prototypes */
extern int yylex();
extern int initLex(int , char **);
 
    

/* external global variables */

extern int yydebug;
extern int yylineno;
extern SymbolTableStackEntryPtr symbolStackTop;
extern int scopeDepth;

/* function prototypes */ 
void yyerror(const char *);

/* global variables */

AstNodePtr program;

ElementPtr tmpElement;

%}

//%error-verbose

/* YYSTYPE */
%union
{
    AstNodePtr nodePtr;
    int        iVal;
    char      *cVal;
    Type      *type;
}

/* terminals */
/* Start adding token names here */
/* Your token names must match Project 1 */
/* The file cmparser.tab.h was gets generated here */

%token	TOK_ELSE
%token	TOK_IF
%token	TOK_RETURN
%token	TOK_VOID
%token	TOK_INT
%token	TOK_WHILE
%token <cVal>	TOK_ID
%token <iVal>	TOK_NUM
%token	TOK_PLUS
%token	TOK_MINUS
%token	TOK_MULT
%token	TOK_DIV
%token	TOK_LT
%token	TOK_LE
%token	TOK_GT
%token	TOK_GE
%token	TOK_EQ
%token	TOK_NE
%token	TOK_ASSIGN
%token	TOK_SEMI
%token	TOK_COMMA
%token	TOK_LPAREN
%token	TOK_RPAREN
%token	TOK_LSQ
%token	TOK_RSQ
%token	TOK_LBRACE
%token	TOK_RBRACE
%token	TOK_ERROR

//TYPES
%type <nodePtr> Fun_Decl_list Fun_Decl
%type <type> Type_Specifier
%type <nodePtr> Compound_Stmt Stmt Compound_scope_Stmt
%type <nodePtr> Expression_Stmt Selection_Stmt Iteration_Stmt Return_Stmt Param Params Param_list
%type <nodePtr> Expression Simple_Expression Add_Expression Factor Var Call 
%type <nodePtr> Term Rel_Op Add_Op Mult_Op
%type <nodePtr> Args_list Args

/* associativity and precedence */
/* specify operator precedence (taken care of by grammar) and associatity here --uncomment */
 
%right	TOK_ASSIGN
%left	TOK_NE TOK_EQ
%nonassoc	TOK_GE TOK_LE TOK_GT TOK_LT
%left	TOK_MINUS TOK_PLUS
%left	TOK_DIV TOK_MULT

%nonassoc TOK_FAKE_ELSE
%nonassoc TOK_ELSE

%nonassoc error


/* Begin your grammar specification here */
%%

Start	:	{initSymbolTable();}	Declarations{/*print_Ast();*/};

Declarations	:	Var_Decl_list Fun_Decl_list;

Var_Decl_list	:	Var_Decl_list Var_Decl 
				| /*empty*/ ;

Fun_Decl_list	:	Fun_Decl_list Fun_Decl
						{
							$$ = $2;
							$1->sibling = $2;
						}
				| Fun_Decl 
					{
						$$ = $1;
						//the ast starts at the first function declaration
						program = $1;
					};

Var_Decl 	:	Type_Specifier TOK_ID TOK_SEMI 
					{
						if(scopeDepth >= 2){
							yyerror("You can declare a variable only in the global scope or at the beginning of the function\n");
						}
						if($1->kind == INT){
							tmpElement = symLookup($2);
							//if we have not already declared the same variable
							//or we declared it in a previous scope
							if(tmpElement == NULL || tmpElement->scope != scopeDepth){
								//we add a new variable to the symbolTable
								symInsert($2,$1,yylineno);
							}else{
								//we throw an error
								yyerror($2);
							}
						}else{
							yyerror($2);
						}
					}

				| Array_Decl;

Type_Specifier	:	TOK_INT 
						{
							//we create the new type int
							$$ = new_type(INT);
						}
				| TOK_VOID
					{
						//we create the new type void
						$$ = new_type(VOID);
					};

Array_Decl	:	Type_Specifier TOK_ID TOK_LSQ TOK_NUM TOK_RSQ TOK_SEMI
					{
						if(scopeDepth >= 2){
							yyerror("You can declare a variable only in the global scope or at the beginning of the function\n");
						}
						if($1->kind == INT){
							//if we have not already declared the same array
							tmpElement = symLookup($2);
							//if we have not already declared the same array
							//or we declared it in a previous scope
							if(tmpElement == NULL || tmpElement->scope != scopeDepth){
								if($1->kind == INT){
									$1->kind = ARRAY;
								}
								//we add a new array to the symbol table and set the dimension
								$1->dimension = $4;
								symInsert($2,$1,yylineno);
							}else{
								//we throw an error
								yyerror($2);
							}
						}else{
							yyerror($2);
						}
					};

Fun_Decl 	:	Type_Specifier TOK_ID
					{
						//if we have not already defined the same function
						if(symLookup($2)==NULL){
							//we create the node for the method declaration
							$<nodePtr>$ = new_Node(METHOD);
							$<nodePtr>$->nSymbolPtr = symInsert($2,$1,yylineno);
							$<nodePtr>$->nSymbolPtr->stype->function = (Type*) malloc(sizeof(Type));
							$<nodePtr>$->nSymbolPtr->stype->function->kind = $1->kind;
							$<nodePtr>$->nSymbolPtr->snode = $<nodePtr>$;
							$<nodePtr>$->nType = $1;
							$1->kind = FUNCTION;
							enterScope();
							//populate the node
							$<nodePtr>$->nSymbolTabPtr = symbolStackTop->symbolTablePtr;
							$<nodePtr>$->nLinenumber = yylineno;
						}else{
							yyerror($2);
						}
					} 
					TOK_LPAREN Params TOK_RPAREN Compound_Stmt 
						{
							//first child node is FORMALVAR
							$<nodePtr>3->children[FIRST] = $5;
							//second child node is COMPOUND_STMT
							$<nodePtr>3->children[SECOND] = $7;
							$$ = $<nodePtr>3;
							leaveScope();
						};

Params	:	TOK_VOID
				{
					//we have no parameters passed to the function
					//the METHOD node has a NULL FORMALVAR child
					$$ = NULL;
				}
		| Param_list
			{
				$$ = $1;
			};

Param_list	:	Param 
					{
						$$ = $1;
					}
			| Param_list TOK_COMMA Param 
				{
					AstNodePtr tmp;
					for(tmp=$1; tmp->sibling != NULL; tmp = tmp->sibling);
					tmp->sibling = $3;
					$$ = $1;
				};

Param 	:	Type_Specifier TOK_ID 
				{
					if($1->kind == INT){
						//we create the parameter node and set to null the sibling
						$$ = new_Node(FORMALVAR);

						tmpElement = symLookup($2);
						if(tmpElement == NULL || tmpElement->scope != scopeDepth){
							//we insert it in the scope
							$$->nSymbolPtr = symInsert($2,$1,yylineno);
							$$->nSymbolPtr->snode = $$;
							$$->nType = $1;
						}else{
							//we throw an error
							yyerror($2);
						}
					}else{
						yyerror($2);
					}
				}
		| Type_Specifier TOK_ID TOK_LSQ TOK_RSQ 
				{
					if($1->kind == INT){
						tmpElement = symLookup($2);
						if(tmpElement == NULL || tmpElement->scope != scopeDepth){
							//we create the parameter node
							$$ = new_Node(FORMALVAR);
							if($1->kind == INT){
								$1->kind = ARRAY;
							}
							//we insert it in the scope
							$$->nSymbolPtr = symInsert($2,$1,yylineno);
							$$->nSymbolPtr->snode = $$;
							$$->nType = $1;
						}else{
							//we throw an error
							yyerror($2);
						}
					}else{
						yyerror($2);
					}
				};

Compound_Stmt	:	TOK_LBRACE Local_Decl_list Stmt_list TOK_RBRACE
						{
							$$ = new_StmtNode(COMPOUND_STMT);
							//only child is the first statement
							$$->children[FIRST] = $<nodePtr>3;
							$$->nSymbolTabPtr = symbolStackTop->symbolTablePtr;
						};

Local_Decl_list	:	Local_Decl_list Local_Decl 
				| /*empty*/;

Stmt_list	:	Stmt_list Stmt
					{
						if($<nodePtr>1 == NULL){
							$<nodePtr>$ = $2;
						}else{
							AstNodePtr tmp;
							for(tmp=$<nodePtr>1; tmp->sibling != NULL; tmp = tmp->sibling);
							tmp->sibling = $2;
							$<nodePtr>$ = $<nodePtr>1;
						}
					}
			| /*empty*/
					{
						$<nodePtr>$ = NULL;
					};

Local_Decl 	:	Var_Decl ;

Stmt 	:	Expression_Stmt	
				{
					$$ = $1;
				}
		| Compound_scope_Stmt
			{
				$$ = $1;
			}
		| Selection_Stmt 
			{
				$$ = $1;
			}
		| Iteration_Stmt
			{
				$$ = $1;
			} 
		| Return_Stmt
			{
				$$ = $1;
			};

Compound_scope_Stmt : TOK_LBRACE 
						{
							enterScope();	
						}
					Local_Decl_list Stmt_list TOK_RBRACE
						{
							$$ = new_StmtNode(COMPOUND_STMT);
							//only child is the first statement
							$$->children[FIRST] = $<nodePtr>4;
							$$->nSymbolTabPtr = symbolStackTop->symbolTablePtr;
							leaveScope();
						};

Expression_Stmt	:	Expression TOK_SEMI 
						{
							$$ = new_StmtNode(EXPRESSION_STMT);
							$$->children[FIRST] = $1;
						}
				| TOK_SEMI
					{
						$$ = new_StmtNode(EXPRESSION_STMT);
						$$->children[FIRST] = new_ExprNode(-1);
					};

Selection_Stmt	:	TOK_IF TOK_LPAREN Expression TOK_RPAREN Stmt 	%prec TOK_FAKE_ELSE 
						{
							$$ = new_StmtNode(IF_THEN_ELSE_STMT);
							$$->children[FIRST] = $3;
							$$->children[SECOND] = $5;
						}
				| TOK_IF TOK_LPAREN Expression TOK_RPAREN Stmt TOK_ELSE Stmt
					{
						$$ = new_StmtNode(IF_THEN_ELSE_STMT);
						$$->children[FIRST] = $3;
						$$->children[SECOND] = $5;
						$$->children[THIRD] = $7;
					};

Iteration_Stmt	:	TOK_WHILE TOK_LPAREN Expression TOK_RPAREN Stmt 
						{
							$$ = new_StmtNode(WHILE_STMT);
							$$->children[FIRST] = $3;
							$$->children[SECOND] = $5;
						};

Return_Stmt	:	TOK_RETURN TOK_SEMI 
					{
						$$ = new_StmtNode(RETURN_STMT);
					}
			| TOK_RETURN Expression TOK_SEMI 
				{
					$$ = new_StmtNode(RETURN_STMT);
					$$->children[FIRST] = $2;
				};

Expression 	:	Var TOK_ASSIGN Expression 
					{
						$$ = new_ExprNode(ASSI_EXP);
						$$->children[FIRST] = $1;
						$$->children[SECOND] = $3;
					}
			| Simple_Expression
				{
					$$ = $1;
				};

Var 	:	TOK_ID 
				{
					$$ = new_ExprNode(VAR_EXP);
					tmpElement = symLookup($1);
					if(tmpElement != NULL){
						$$->nSymbolPtr = tmpElement;
					}else{
						yyerror($1);
					}
						
				}
		| TOK_ID TOK_LSQ Simple_Expression TOK_RSQ
			{
				$$ = new_ExprNode(ARRAY_EXP);
				tmpElement = symLookup($1);
				if(tmpElement != NULL){
					$$->nSymbolPtr = tmpElement;
					$$->children[FIRST] = $3;
				}else{
					yyerror($1);
				}
			} ;

Simple_Expression	:	Add_Expression Rel_Op Add_Expression
							{
								$$ = $2;
								$2->children[FIRST] = $1;
								$2->children[SECOND] = $3;
							}
					| Add_Expression
						{
							$$ = $1;
						};

Rel_Op	:	TOK_EQ 
				{
					$$ = new_ExprNode(EQ_EXP);
				}
		| TOK_NE 
			{
				$$ = new_ExprNode(NE_EXP);
			}
		| TOK_LT 
			{
				$$ = new_ExprNode(LT_EXP);
			}
		| TOK_LE 
			{
				$$ = new_ExprNode(LE_EXP);
			}
		| TOK_GT 
			{
				$$ = new_ExprNode(GT_EXP);
			}
		| TOK_GE
			{
				$$ = new_ExprNode(GE_EXP);
			};

Add_Expression	:	Add_Expression Add_Op Term 
						{
							$$ = $2;
							$$->children[FIRST] = $1;
							$$->children[SECOND] = $3;
						}
				| Term 
					{
						$$ = $1;
					};

Add_Op	:	TOK_PLUS 
				{
					$$ = new_ExprNode(ADD_EXP);
				}
		| TOK_MINUS
			{
				$$ = new_ExprNode(SUB_EXP);
			} ;

Term 	:	Term Mult_Op Factor 
				{
					$$ = $2;
					$2->children[FIRST] = $1;
					$2->children[SECOND] = $3;
				}
		| Factor 
			{
				$$ = $1;
			};

Mult_Op	:	TOK_MULT 
				{
					$$ = new_ExprNode(MULT_EXP);
				}
		| TOK_DIV 
			{
				$$ = new_ExprNode(DIV_EXP);
			};

Factor	:	TOK_LPAREN Simple_Expression TOK_RPAREN 
				{
					$$ = $2;
				}
		| Var 
			{
				$$ = $1;
			}
		| Call 
			{
				$$ = $1;
			}
		| TOK_NUM 
			{
				$$ = new_ExprNode(CONST_EXP);
				$$->nValue = $1;
			};

Call 	:	TOK_ID TOK_LPAREN Args TOK_RPAREN
				{
					$$ = new_ExprNode(CALL_EXP);
					$$->nLinenumber = yylineno;
					$$->nSymbolPtr = symLookup($1);
					$$->children[FIRST] = $3;
					$$->fname = strdup($1);
				};

Args	:	Args_list
				{
					$$ = $1;
				} 
		| /*empty*/
				{
					$$ = NULL;
				};

Args_list	:	Args_list TOK_COMMA Simple_Expression 
					{
						AstNodePtr tmp;
						for(tmp=$<nodePtr>1; tmp->sibling != NULL; tmp = tmp->sibling);
						tmp->sibling = $3;
						$$ = $1;
					}
			| Simple_Expression 
				{
					$$ = $1;
				};

%%
void yyerror (char const *s) {
       fprintf (stderr, "Line %d: %s\n", yylineno, s);
       exit(1);
}

/*
int main(int argc, char **argv){

	initLex(argc,argv);

#ifdef YYLLEXER
   while (gettok() !=0) ; //gettok returns 0 on EOF
    return;
#else
    yyparse();
    typecheck(program);
#endif
    
} */
