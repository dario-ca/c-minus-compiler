#include "symbolTable.h"
#include "ast.h"


extern SymbolTableStackEntryPtr symbolStackTop;

extern int scopeDepth;
extern int yylineno;

//creates a new expression node
AstNodePtr  new_ExprNode(ExpKind kind){
	AstNodePtr newAstExprNode = new_Node(EXPRESSION);
	newAstExprNode->eKind = kind;
	return newAstExprNode;
}

//creates a new statement node
AstNodePtr new_StmtNode(StmtKind kind){
	AstNodePtr newAstStmtNode = new_Node(STMT);
	newAstStmtNode->sKind = kind;
	return newAstStmtNode;
}

//creates a new node, of any type
AstNodePtr new_Node(NodeKind nKind){
	AstNodePtr newAstNode = (AstNodePtr) malloc (sizeof(AstNode));
	newAstNode->nKind = nKind;
	newAstNode->sibling = NULL;
	int i;
	for(i=0;i<MAXCHILDREN;i++)
		newAstNode->children[i] = NULL;
	newAstNode->nLinenumber = yylineno;
	return newAstNode;
}

//creates a new type node for entry into symbol table
Type* new_type(TypeKind kind){
	Type* ty = (Type*)malloc(sizeof(Type));
	ty->kind = kind;
	return ty;
}