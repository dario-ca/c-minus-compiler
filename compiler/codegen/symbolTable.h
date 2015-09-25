
#ifndef _SYMBOL_TABLE
#define _SYMBOL_TABLE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "globals.h"


/* data structure of one element in the symbol table */

typedef enum  {VOID, INT, ARRAY, FUNCTION} TypeKind;

struct type {
	TypeKind kind;   /* one from the enum above */
	int  dimension; /* for arrays */
	struct type *function; /*function argument and return types */
};

typedef struct type Type;

typedef struct type *TypePtr;

struct element {
	int     key;
	char       *id;
	int             linenumber;
	int             scope;   /* scope depth at declaration */
	Type *stype;        /* pointer to the type infomation */
	struct element  *next;      /* pointer to the next symbol with the same hash table index */
	struct node     *snode;   /* AST Node for method declaration and formal vars*/
	struct element *queue_next; /* Pointer to next element in  queue of  declared elements  */

};

typedef struct element Element;

typedef struct element *ElementPtr;

typedef ElementPtr HashTableEntry;


/* data structure of the symbol table */

struct symbolTable {
	HashTableEntry  hashTable[MAXHASHSIZE];     /* hash table  */
	struct element *queue; /* points to first inserted (or first declared) symbol */

};

typedef struct symbolTable *SymbolTablePtr;

struct symbolTableStackEntry {
	SymbolTablePtr          symbolTablePtr;
	struct symbolTableStackEntry    *prevScope; //previous scope
} SymbolTableStackEntry;

typedef struct symbolTableStackEntry *SymbolTableStackEntryPtr;


void         initSymbolTable() ;
ElementPtr      symLookup(char *);
ElementPtr      symInsert(char *, struct type *, int );
void         enterScope();
void            leaveScope();
void printSymbolTable(int);


#endif
