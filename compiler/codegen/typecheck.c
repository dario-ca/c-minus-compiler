
#include "symbolTable.h"
#include "ast.h"
#include "typecheck.h"

#define FIRST 0
#define SECOND 1
#define THIRD 2

//int returnPresent = 0;
extern AstNodePtr program;
// Starts typechecking the AST  returns 1 on success
//use the global variable program

int typecheck(){
	if(program == NULL){
		exit(1);
	}

	AstNodePtr iterator;
	for(iterator = program; iterator != NULL; iterator = iterator->sibling){
		//fprintf(stderr,"Typechecking method: %s\n",iterator->nSymbolPtr->id);
		typecheck_method(iterator);
	};
	//fprintf(stderr,"All methods typechecked: OK\n");
	return 1;
}

// compares two types and returns the resulting type
// from the comparison

Type* type_equiv(Type *t1, Type *t2){
	if(t1->kind == t2->kind){
		Type *newType = (Type*)malloc(sizeof(Type));
		newType->function = NULL;
		newType->kind = t1->kind;
		return newType;
	}
	return NULL;
}

// Typechecks a method and returns 1 on success
int typecheck_method(AstNode *node){
	if(node->nKind != METHOD){
		typeError(node->nLinenumber, "Node is not type METHOD");
	}
	//returnPresent = 0;
	AstNodePtr tmpStmt; 
	AstNodePtr tmpCompound = node->children[SECOND];
	for(tmpStmt = tmpCompound->children[FIRST]; tmpStmt->sibling != NULL; tmpStmt = tmpStmt->sibling){
		typecheck_stmt(tmpStmt, node->nSymbolPtr->stype->function);
	}
	typecheck_stmt(tmpStmt, node->nSymbolPtr->stype->function);
	if(tmpStmt->sKind != RETURN_STMT){
		typeError(node->nLinenumber, "Function must have a return statement");
	}
	
	return 1;
}

// Typechecks a statement and returns 1 on success
int typecheck_stmt(AstNode *node, Type* returnType){
	if (node->nKind != STMT){
		typeError(node->nLinenumber, "Node is not type STMT");
	}else{
		StmtKind stmtKind = node->sKind;

		switch(stmtKind){

			case COMPOUND_STMT:{
				AstNodePtr tmpStmt;
				for (tmpStmt = node->children[FIRST]; tmpStmt != NULL; tmpStmt = tmpStmt->sibling) {
					typecheck_stmt(tmpStmt, returnType);
				}
			}break;

			case RETURN_STMT:{
				//returnPresent = 1;
				if(node->children[FIRST] == NULL && returnType->kind != VOID){
						typeError(node->nLinenumber, "Returning void while function returns int");
				}else if(node->children[FIRST] != NULL){
					Type* tempType = typecheck_expr(node->children[FIRST]);
					if(type_equiv(returnType, tempType) == NULL){
						typeError(node->nLinenumber, "Function must return int");
					}
				}
			}break;

			case EXPRESSION_STMT:{
				typecheck_expr(node->children[FIRST]);
			}break;

			case IF_THEN_ELSE_STMT:{
				Type * cmpType = typecheck_expr(node->children[FIRST]); 
				if(cmpType->kind == INT){
					typecheck_stmt(node->children[SECOND], returnType);
					if(node->children[THIRD] != NULL){
						typecheck_stmt(node->children[THIRD], returnType);
					}
				}else{
					typeError(node->children[FIRST]->nLinenumber, "Condition must be an integer expression");
				}
			}break;

			case WHILE_STMT:{
				Type * cmpType = typecheck_expr(node->children[FIRST]); 
				if(cmpType->kind == INT){
					typecheck_stmt(node->children[SECOND], returnType);
				}else{
					typeError(node->children[FIRST]->nLinenumber, "Condition must be an integer expression");
				}
			}break;

		}

	}
	return 1;
}

// Type checks a given expression and returns its type
Type *typecheck_expr (AstNode *node){
	if (node->nKind != EXPRESSION){
		typeError(node->nLinenumber, "Node is not an EXPRESSION");
	}else{
		Type *resultType;
		ExpKind exprKind = node->eKind;
		
		switch(exprKind){
			case VAR_EXP:{
				if(node->nSymbolPtr->stype->kind == INT || node->nSymbolPtr->stype->kind == ARRAY){
					resultType = node->nSymbolPtr->stype;
				}else{
					typeError(node->nLinenumber, "Variable is not an integer nor an array");
				}
			} break;
			
			case ARRAY_EXP:{
				if(node->nSymbolPtr->stype->kind == ARRAY){
					Type * indexType = typecheck_expr(node->children[FIRST]);
					if( indexType->kind == INT){
						resultType = indexType;
					}else{
						typeError(node->nLinenumber, "Array index is not an integer expression");
					}
				}else{
					typeError(node->nLinenumber, "Expression is not an array");
				}
			} break;
			
			case ASSI_EXP:{
				AstNodePtr lValue = node->children[FIRST];
				if (lValue->eKind == VAR_EXP || lValue->eKind == ARRAY_EXP){
					resultType = type_equiv(typecheck_expr(lValue),typecheck_expr(node->children[SECOND]));
					if(resultType == NULL){
						typeError(node->nLinenumber, "Assignment is done between expressions of different types");
					}
				}else{
					typeError(node->nLinenumber, "Assignment is not between integers nor array elements");
				}
			}break;

			case ADD_EXP:
			case SUB_EXP:
			case MULT_EXP:
			case DIV_EXP:
			case GT_EXP:
			case LT_EXP:
			case GE_EXP:
			case LE_EXP:{
				Type *tempType1 = typecheck_expr(node->children[FIRST]);
				Type *tempType2 = typecheck_expr(node->children[SECOND]);
				if (tempType1->kind == INT && tempType2->kind == INT){
					resultType = type_equiv(tempType1, tempType2);
					if(resultType == NULL){
						typeError(node->nLinenumber, "Binary expression between different types");
					}
				}else{
					typeError(node->nLinenumber, "Binary expression allowed only between integer expressions");
				}
			}break;

			case EQ_EXP:
			case NE_EXP:{
				resultType = type_equiv(typecheck_expr(node->children[FIRST]),typecheck_expr(node->children[SECOND]));
				if(resultType == NULL){
					typeError(node->nLinenumber, "Equivalence is allowed only between integer expressions");
				}
				
				Type *newType = (Type*)malloc(sizeof(Type));
				newType->function = NULL;
				newType->kind = INT;
				resultType = newType;
			}break;

			case CALL_EXP:{
				ElementPtr functionCalled = symLookup(node->fname);
				if(functionCalled == NULL){
					typeError(node->nLinenumber, "Function not defined");
				}
				if(functionCalled->stype->kind == FUNCTION){
					int numArg;
					int numParam;
					AstNodePtr tmpNodeArg;
					AstNodePtr tmpNodeParam;
					Type * argType;
					Type * paramType;

					//count number of arguments given and number of parameters requested
					for(numArg = 0, tmpNodeArg = node->children[FIRST]; tmpNodeArg != NULL; tmpNodeArg = tmpNodeArg->sibling,numArg++);
					for(numParam = 0, tmpNodeParam = functionCalled->snode->children[FIRST]; tmpNodeParam != NULL; tmpNodeParam = tmpNodeParam->sibling,numParam++);
					if(numArg == numParam){
						//check that arguments types and parameters types match
						for(tmpNodeArg = node->children[FIRST],  tmpNodeParam = functionCalled->snode->children[FIRST];
							tmpNodeArg != NULL && tmpNodeParam != NULL;
							tmpNodeArg = tmpNodeArg->sibling, tmpNodeParam = tmpNodeParam->sibling){
								argType = typecheck_expr(tmpNodeArg);
								if(argType->kind != tmpNodeParam->nType->kind){
									typeError(node->nLinenumber, "Types of actual arguments do not match the type of formal parameters");
								}
						}
					resultType = functionCalled->stype->function;
					}else{
						typeError(node->nLinenumber, "Number of actual arguments does not match the number of formal parameters");
					}
				}else{
					typeError(node->nLinenumber, "Function called is not a function");
				}
			}break;

			case CONST_EXP:{
				Type *intType = (Type*) malloc (sizeof(Type));
				intType->kind = INT;
				resultType = intType;
			}break;

		}
		return resultType;
	}
	return NULL;
}

void typeError(int line, char * message){
	fprintf(stderr, "Line %d: %s\n",line,message);
	exit(1);
}


