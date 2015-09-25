#include "symbolTable.h"

// Top should point to the top of the scope stack,
// which is the most recent scope pushed

SymbolTableStackEntryPtr symbolStackTop;

int scopeDepth;


/* global function prototypes */

//allocate the global scope entry and symbol table --and set scopeDepth to 0
// The global scope remains till the end of the program
// return 1 on success, 0 on failure (such as a failed malloc)
void initSymbolTable(){
	//pointer to global scope in stack
	SymbolTableStackEntryPtr initStackGlobalScopeEntry = (SymbolTableStackEntryPtr) malloc(sizeof(struct symbolTableStackEntry));
	if(initStackGlobalScopeEntry == NULL)
		return;
	//symble table of the global scope
	initStackGlobalScopeEntry->symbolTablePtr = (SymbolTablePtr) malloc(sizeof(struct symbolTable));
	if(initStackGlobalScopeEntry->symbolTablePtr == NULL)
			return;
	int i;
	for(i=0; i<MAXHASHSIZE; i++){
		initStackGlobalScopeEntry->symbolTablePtr->hashTable[i] = NULL;
	}
	scopeDepth=0; //global scope is level 0
	initStackGlobalScopeEntry->prevScope=NULL; //no scope before global one
	initStackGlobalScopeEntry->symbolTablePtr->queue = NULL;
	symbolStackTop=initStackGlobalScopeEntry; //top of stack is global scope
	//printf("Init: %d\n",symbolStackTop);
	return;
}


int hash(char *id) {
	int i;
	int sum = 0;
	for (i = 0; id[i] != '\0'; i++) {
		sum += id[i];
	}
	return sum % MAXHASHSIZE;
}

// Look up a given entry
ElementPtr symLookup(char *name){
	int hash_key = hash(name);
	ElementPtr tmp_element=NULL;
	SymbolTableStackEntryPtr tmp_scope=NULL;
	int tmp_scopeDepth=scopeDepth;
	//we start from the scope on top of the stack and we go to the outer scope if we do not find the element
	//we stop at the global scope at depth 0
	for(tmp_scope=symbolStackTop;tmp_scopeDepth >= 0; tmp_scopeDepth--,tmp_scope=tmp_scope->prevScope){
		//we go to the element in hash position of the current symbol table
		//we scan all the elements in that position until we find it or we reach the end of the list
		for(tmp_element=tmp_scope->symbolTablePtr->hashTable[hash_key]; tmp_element != NULL; 
			tmp_element=tmp_element->next){
				//we found the element
				if(strcmp(tmp_element->id,name)==0){
					//printf("Found: %s\n",tmp_element->id);
					return tmp_element;
				}
		}
	}
	//we did not find the element in any scope
	//printf("Element not found\n");
	return NULL;
}


// Insert an element with a specified type in a particular line number
// initialize the scope depth of the entry from the global var scopeDepth
ElementPtr symInsert(char *name, struct type *type, int line){
	//pointer to the new element to add
	ElementPtr newElement = (ElementPtr) malloc(sizeof(Element));
	if(newElement == NULL)
		return NULL;

	newElement->key = hash(name);
	newElement->id = strdup(name);
	newElement->linenumber = line;
	newElement->scope = scopeDepth;
	newElement->stype = type;
	newElement->snode = NULL;
	newElement->queue_next = NULL;
	if(symbolStackTop->symbolTablePtr->queue == NULL){
		//if(type->kind != FUNCTION)
			symbolStackTop->symbolTablePtr->queue = newElement;
	}else{
		ElementPtr tmp;
		for(tmp=symbolStackTop->symbolTablePtr->queue;tmp->queue_next != NULL; tmp = tmp->queue_next);
		tmp->queue_next = newElement;
	}
	//the new element points to the old head of list of elements in the symbol table
	newElement->next = symbolStackTop->symbolTablePtr->hashTable[newElement->key];
	//the new element is the new head of the list of elements in the symbol table
	symbolStackTop->symbolTablePtr->hashTable[newElement->key] = newElement;

	//printf("Inserted: %s\n",newElement->id);
	return newElement;
}


//push a new entry to the symbol stack
// This should modify the variable top and change the scope depth
// return 1 on success, 0 on failure (such as a failed malloc)
void enterScope(){
	//pointer to scope in stack
	SymbolTableStackEntryPtr stackScopeEntry = (SymbolTableStackEntryPtr) malloc(sizeof(struct symbolTableStackEntry));
	if(stackScopeEntry == NULL)
		return;
	//symble table of the scope
	stackScopeEntry->symbolTablePtr = (SymbolTablePtr) malloc(sizeof(struct symbolTable));
	if(stackScopeEntry->symbolTablePtr == NULL)
			return;
	
	scopeDepth++; //scope depth increment
	stackScopeEntry->prevScope=symbolStackTop; //scope before new scope is the current one
	symbolStackTop=stackScopeEntry; //top of stack is new scope
	
	int i;
	for(i=0; i<MAXHASHSIZE; i++){
		stackScopeEntry->symbolTablePtr->hashTable[i] = NULL;
	}
	stackScopeEntry->symbolTablePtr->queue = NULL;


	//printf("New scope: %d\n",symbolStackTop);
	return;
}


//pop an entry off the symbol stack
// This should modify top and change scope depth
void leaveScope(){
	
	SymbolTableStackEntryPtr tmp = symbolStackTop;
	symbolStackTop = tmp->prevScope;
	//printf("Removed scope: %d\n",tmp);
	free(tmp);
	scopeDepth--;
	//printf("New top: %d\n",symbolStackTop);
	return;
}

// Do not modify this function
void printElement(ElementPtr symelement) {
	if (symelement != NULL) {
		printf("*****Line %d: %s\n", symelement->linenumber, symelement->id);
	}
	else printf("Wrong call! symbol table entry NULL");
	return;
}

//should traverse through the entire symbol table and print it
// must use the printElement function given above
void printSymbolTable(int flag){
	ElementPtr tmp_element=NULL;
	SymbolTableStackEntryPtr current_scope=NULL;
	int tmp_scopeDepth=scopeDepth;
	for(current_scope=symbolStackTop;tmp_scopeDepth >= 0 && current_scope != NULL; tmp_scopeDepth--,current_scope=current_scope->prevScope){
		for(tmp_element = current_scope->symbolTablePtr->queue; tmp_element!=NULL; tmp_element=tmp_element->queue_next){
			printElement(tmp_element);
		}
	}
	return;
}