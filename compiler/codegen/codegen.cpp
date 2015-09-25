//#include "llvm/Analysis/Passes.h"
#include "llvm/IR/Verifier.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
//#include "llvm/ExecutionEngine/JIT.h"
//#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
//#include "llvm/IR/PassManager.h"
#include "llvm/Support/TargetSelect.h"
//#include "llvm/Transforms/Scalar.h"
//#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include <cstdio>
#include <map>
#include <string>
#include <vector>
namespace project473 {
  extern "C" {
    #include "ast.h"
  }
}

using namespace llvm;

extern "C" int initLex(int argc, char** argv);
extern "C" void initSymbolTable();  
extern "C" int  yyparse();
extern "C" int typecheck();
extern "C" int gettoken();
extern "C" void print_Ast();
extern "C" void printSymbolTable(int flag); 
extern "C" project473::AstNodePtr program;
extern "C" project473::SymbolTableStackEntryPtr symbolStackTop;

static Module *TheModule;
static IRBuilder<> Builder(getGlobalContext());
static std::map<std::string, AllocaInst*> NamedValues;
static std::map<std::string, Function*> Functions;
BasicBlock *ReturnBB=0;
AllocaInst *RetAlloca=0;
bool isReturnPresent = false;
int scopeCount = 0;
void Error(const char *Str) { fprintf(stderr, "Error: %s\n", Str);}
Value *ErrorV(const char *Str) { Error(Str); return 0; }

///Codegen the Allocas for local variables that correspond to the formal variables of function F.
///function_node is an AST node representing a function. Steps
///1. For every formal variable, get its name from the AST, create a Type object
// and call AllocaInst* alloca_inst = Builder.CreateAlloca(Type, name);
//2. set NamedValues[name] = alloca_inst
//3. For every formal variable of the function, create a store instruction that copies the value
//from the formal variable to the allocated local variable.
void CreateFormalVarAllocations(Function *F, project473::AstNodePtr function_node) {

  char *name;
  llvm::IntegerType *intType = Builder.getInt32Ty();
  for(project473::AstNodePtr formalVariable = function_node->children[0]; formalVariable != NULL;
      formalVariable = formalVariable->sibling){
    
    llvm::StringRef name(strdup(formalVariable->nSymbolPtr->id));
    if(formalVariable->nSymbolPtr->stype->kind == project473::INT){
      NamedValues[name] = Builder.CreateAlloca(intType, 0, name);
    }else if(formalVariable->nSymbolPtr->stype->kind == project473::ARRAY){
      llvm::PointerType *arrType = llvm::PointerType::get(intType,0);
      NamedValues[name] = Builder.CreateAlloca(arrType, 0, name+".addr");
    }
    NamedValues[name]->setAlignment(4);
  }
  
  llvm::StringRef it_name;
  for(Function::arg_iterator formalParameter = F->arg_begin(); formalParameter != F->arg_end() ; formalParameter++){
    it_name = formalParameter->getName();
    
    Builder.CreateStore(&*formalParameter,NamedValues[it_name])->setAlignment(4);
  }
  return;
}

Function* Codegen_Function_Declaration(project473::AstNodePtr declaration_node) {
  char* fname = strdup(declaration_node->nSymbolPtr->id); 
  std::string Name(fname);
  std::vector<Type*> formalvars;
  project473::AstNodePtr formalVar = declaration_node->children[0]; 
  while(formalVar) { 
    if(formalVar->nSymbolPtr->stype->kind == project473::INT) {
      formalvars.push_back(Type::getInt32Ty(getGlobalContext()));
      formalVar=formalVar->sibling;
    }
    else if(formalVar->nSymbolPtr->stype->kind == project473::ARRAY){
      formalvars.push_back(Type::getInt32PtrTy(getGlobalContext()));
      formalVar=formalVar->sibling;
    }else{
      printf("Error, formal variable is not an int nor an array, in line: %d", formalVar->nLinenumber);
    }
  }
  project473::Type* functionTypeList = declaration_node->nSymbolPtr->stype->function;
  FunctionType *FT;
  if(functionTypeList->kind==project473::INT) { 
     FT = FunctionType::get(Type::getInt32Ty(getGlobalContext()), formalvars, false);
  }
  else if(functionTypeList->kind==project473::VOID) {
    FT = FunctionType::get(Type::getVoidTy(getGlobalContext()), formalvars, false);
  }

  Function *F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule);
  // Set names for all arguments. Reuse formalVar
  formalVar = declaration_node->children[0];
  for (Function::arg_iterator AI = F->arg_begin(); formalVar != NULL; ++AI, formalVar=formalVar->sibling) {
          std::string argName(formalVar->nSymbolPtr->id);
          AI->setName(argName);
  }
  Functions[Name] = F; //add the Function to the map of functions
  return F;
}

///Generate code for expressions
Value* Codegen_Expression(project473::AstNodePtr expression) {
    Value *return_value;
    
    if(expression->eKind==project473::ASSI_EXP) {
      Value *lhs;
      Value *rhs = Codegen_Expression(expression->children[1]);
      if(expression->children[0]->eKind==project473::ARRAY_EXP){
        Value* indexVal = Codegen_Expression(expression->children[0]->children[0]);
        char *name = expression->children[0]->nSymbolPtr->id;
        llvm::StringRef ArrName(name);
        Value *Zero = Builder.getInt32(0);
        Value *Args[] = { Zero, indexVal };
        if(NamedValues[ArrName]){
          AllocaInst *arrayAlloca = NamedValues[ArrName];
          //lhs = Builder.CreateInBoundsGEP(arrayAlloca, Args, "arrayidx");
          Value * pointerElement;
          if(arrayAlloca->getAllocatedType()->isArrayTy()){//local array declaration
            lhs = Builder.CreateInBoundsGEP(arrayAlloca,Args, "arrayidx");
          }else{//formal parameter
            LoadInst *loadedArrPtr = Builder.CreateLoad(arrayAlloca,ArrName);
            loadedArrPtr->setAlignment(4);
            lhs = Builder.CreateInBoundsGEP(Builder.getInt32Ty(),loadedArrPtr, indexVal,"arraidx");
          }
        }else{
          llvm::GlobalVariable* arrayAlloca = TheModule->getNamedGlobal(ArrName);
          lhs = Builder.CreateInBoundsGEP(arrayAlloca, Args, "arrayidx");
        }
        return_value = Builder.CreateStore(rhs, lhs);
        cast<llvm::StoreInst>(return_value)->setAlignment(4);
      }else{
        char *lhs_name = strdup(expression->children[0]->nSymbolPtr->id);
        llvm::StringRef varName(lhs_name);
        if(NamedValues[varName]){//local
          lhs = NamedValues[varName];
          return_value = Builder.CreateStore(rhs, lhs);
        }else{//global
          llvm::GlobalVariable *lhs = TheModule->getNamedGlobal(varName);
          return_value = Builder.CreateStore(rhs, lhs);
        }
        cast<llvm::StoreInst>(return_value)->setAlignment(4);
        return_value = Builder.getInt32(1);
      }
    }
   
    else if(expression->eKind==project473::VAR_EXP) {
        char *name = strdup(expression->nSymbolPtr->id);
        llvm::StringRef varName(name);
        if(NamedValues[varName]){//is a local variable
          if(expression->nSymbolPtr->stype->kind == project473::ARRAY 
                  && NamedValues[varName]->getAllocatedType()->isArrayTy()){
            
            Value *Args[] = {Builder.getInt32(0),Builder.getInt32(0)};
            return_value = Builder.CreateInBoundsGEP(NamedValues[varName],Args,"arraydecay");
          }else{
            AllocaInst* pointerToLocal = NamedValues[varName];
            return_value = Builder.CreateLoad(pointerToLocal,varName);
            cast<llvm::LoadInst>(return_value)->setAlignment(4);
          }
        }else{//is a global variable
          if(expression->nSymbolPtr->stype->kind == project473::ARRAY){
            Value *Args[] = {Builder.getInt32(0),Builder.getInt32(0)};
            return_value = Builder.CreateInBoundsGEP(TheModule->getNamedGlobal(varName),Args,"arraydecay");
          }else{
            llvm::GlobalVariable* gVar = TheModule->getNamedGlobal(varName);
            return_value = Builder.CreateLoad(gVar,varName);
            cast<llvm::LoadInst>(return_value)->setAlignment(4);
          }
        }
    }
   
    else if(expression->eKind==project473::ARRAY_EXP) {
      Value* indexVal = Codegen_Expression(expression->children[0]);
      char *name = expression->nSymbolPtr->id;
      llvm::StringRef ArrName(name);
      Value *Zero = Builder.getInt32(0);
      Value *Args[] = { Zero, indexVal};
      if(NamedValues[ArrName]){
        AllocaInst *arrayAlloca = NamedValues[ArrName];
        Value * pointerElement;
        if(arrayAlloca->getAllocatedType()->isArrayTy()){
          pointerElement = Builder.CreateInBoundsGEP(arrayAlloca,Args, "arrayidx");
        }else{
          LoadInst *loadedArrPtr = Builder.CreateLoad(arrayAlloca);
          loadedArrPtr->setAlignment(4);
          pointerElement = Builder.CreateInBoundsGEP(Builder.getInt32Ty(),loadedArrPtr, indexVal,"arraidx");
        }
        return_value = Builder.CreateLoad(pointerElement,ArrName);
        cast<llvm::LoadInst>(return_value)->setAlignment(4);
      }else{
        llvm::GlobalVariable* arrayAlloca = TheModule->getNamedGlobal(ArrName);
        Value* pointerElement = Builder.CreateInBoundsGEP(arrayAlloca, Args, "arrayidx");
        return_value = Builder.CreateLoad(pointerElement,ArrName);
        cast<llvm::LoadInst>(return_value)->setAlignment(4);
      }
    }

    else if(expression->eKind==project473::CONST_EXP) {
      return_value = Builder.getInt32(expression->nValue);
    }
    
    else if(expression->eKind==project473::GT_EXP) {
      Value *operand0 = Codegen_Expression(expression->children[0]);
      Value *operand1 = Codegen_Expression(expression->children[1]);
      return_value = Builder.CreateICmpSGT(operand0,operand1,"cmp");

    }
    
    else if(expression->eKind==project473::LT_EXP) {
      Value *operand0 = Codegen_Expression(expression->children[0]);
      Value *operand1 = Codegen_Expression(expression->children[1]);
      return_value = Builder.CreateICmpSLT(operand0,operand1,"cmp");
    }

    else if(expression->eKind==project473::GE_EXP) {
      Value *operand0 = Codegen_Expression(expression->children[0]);
      Value *operand1 = Codegen_Expression(expression->children[1]);
      return_value = Builder.CreateICmpSGE(operand0,operand1,"cmp");
    }

    else if(expression->eKind==project473::LE_EXP) {
      Value *operand0 = Codegen_Expression(expression->children[0]);
      Value *operand1 = Codegen_Expression(expression->children[1]);
      return_value = Builder.CreateICmpSLE(operand0,operand1,"cmp");
    }
    
    else if(expression->eKind==project473::EQ_EXP) {
      Value *operand0 = Codegen_Expression(expression->children[0]);
      Value *operand1 = Codegen_Expression(expression->children[1]);
      return_value = Builder.CreateICmpEQ(operand0,operand1,"cmp");
    }

    else if(expression->eKind==project473::NE_EXP) {
      Value *operand0 = Codegen_Expression(expression->children[0]);
      Value *operand1 = Codegen_Expression(expression->children[1]);
      return_value = Builder.CreateICmpNE(operand0,operand1,"cmp");
    }

    else if(expression->eKind==project473::ADD_EXP) {
       Value *operand0 = Codegen_Expression(expression->children[0]);
       Value *operand1 = Codegen_Expression(expression->children[1]);
       return_value = Builder.CreateAdd(operand0, operand1, "add", false, false);
    }
    
    else if(expression->eKind==project473::SUB_EXP) {
       Value *operand0 = Codegen_Expression(expression->children[0]);
       Value *operand1 = Codegen_Expression(expression->children[1]);
       return_value = Builder.CreateSub(operand0, operand1, "sub", false, false);
    }

    else if(expression->eKind==project473::MULT_EXP) {
       Value *operand0 = Codegen_Expression(expression->children[0]);
       Value *operand1 = Codegen_Expression(expression->children[1]);
       return_value = Builder.CreateMul(operand0, operand1, "mul", false, false);
    }
    
    else if(expression->eKind==project473::DIV_EXP) {
       Value *operand0 = Codegen_Expression(expression->children[0]);
       Value *operand1 = Codegen_Expression(expression->children[1]);
       return_value = Builder.CreateUDiv(operand0, operand1, "div", false);
    }
    
    else if(expression->eKind==project473::CALL_EXP) {
      char *fname = expression->fname;
      llvm::StringRef FName(fname);
      Function *Callee = Functions[FName];
      std::vector<llvm::Value*> actualArgs;
      for(project473::AstNodePtr actualArgument = expression->children[0]; actualArgument != NULL;
          actualArgument = actualArgument->sibling){
        actualArgs.push_back(Codegen_Expression(actualArgument));
      }
      llvm::ArrayRef<Value*> Args(actualArgs);
      if(Callee->getReturnType()->isVoidTy()){
        return_value = Builder.CreateCall(Callee,Args);
      }else{
        return_value = Builder.CreateCall(Callee,Args,"call");
      }
    }

  return return_value;
}

///Generates code for the statements.
//Calls Codegen_Expression
Value* Codegen_Statement(Function *TheFunction, project473::AstNodePtr statement) {
    Value *return_value;
    
    if(statement->sKind==project473::EXPRESSION_STMT) {
          return_value = Codegen_Expression(statement->children[0]);
    }

    else if(statement->sKind==project473::RETURN_STMT) {
      llvm::Type *returnType = TheFunction->getReturnType();
      if(returnType->isIntegerTy()){
        Value *exprVal = Codegen_Expression(statement->children[0]);
        if(ReturnBB == 0){//returnBB not inserted
          if(scopeCount == 0 && isReturnPresent == false){
            return_value = Builder.CreateRet(exprVal);
          }else if(scopeCount > 0){
            llvm::IntegerType *intType = Builder.getInt32Ty();
            RetAlloca = Builder.CreateAlloca(intType, 0, "retval");
            RetAlloca->setAlignment(4);
            Builder.CreateStore(exprVal, RetAlloca)->setAlignment(4);
            ReturnBB = BasicBlock::Create(getGlobalContext(), "return", TheFunction);
            return_value = Builder.CreateBr(ReturnBB);
          }
        }else{//returnBB already inserted
          llvm::IntegerType *intType = Builder.getInt32Ty();
          RetAlloca = Builder.CreateAlloca(intType, 0, "retval");
          RetAlloca->setAlignment(4);
          Builder.CreateStore(exprVal, RetAlloca)->setAlignment(4);
          return_value = Builder.CreateBr(ReturnBB);
        }
      }else if(returnType->isVoidTy()){
        if(ReturnBB == 0){//returnBB not inserted
          if(scopeCount == 0 && isReturnPresent == false){
            return_value = Builder.CreateRetVoid();
          }else if(scopeCount > 0){
            ReturnBB = BasicBlock::Create(getGlobalContext(), "return", TheFunction);
            return_value = Builder.CreateBr(ReturnBB);
          }
        }else{//returnBB already inserted
          return_value = Builder.CreateBr(ReturnBB);
        }
      }
      isReturnPresent = true;
    }
    
    else if(statement->sKind==project473::IF_THEN_ELSE_STMT) {
      scopeCount++;
      Value* condition = Codegen_Expression(statement->children[0]);
      BasicBlock *ThenBB = BasicBlock::Create(getGlobalContext(), "if.t", TheFunction);
      BasicBlock *ElseBB = BasicBlock::Create(getGlobalContext(), "if.e");
      BasicBlock *MergeBB = BasicBlock::Create(getGlobalContext(), "if.end");
      if(statement->children[2] != NULL){//ELSE is present
        if(dyn_cast<llvm::ConstantInt>(condition)){//condition is constant
          if(cast<llvm::ConstantInt>(condition)->isZero()){
            Builder.CreateBr(ElseBB);
          }else{
            Builder.CreateBr(ThenBB);
          }
        }else{
          if(condition->getType() == Builder.getInt32Ty()){
            condition = Builder.CreateICmpNE(condition,Builder.getInt32(0),"tobool");
          }
          Builder.CreateCondBr(condition, ThenBB, ElseBB);
        }
      }else{//ELSE is not present
        if(dyn_cast<llvm::ConstantInt>(condition)){//condition is constant
          if(cast<llvm::ConstantInt>(condition)->isZero()){
            Builder.CreateBr(MergeBB);
          }else{
            Builder.CreateBr(ThenBB);
          }
        }else{
          if(condition->getType() == Builder.getInt32Ty()){
            condition = Builder.CreateICmpNE(condition,Builder.getInt32(0),"tobool");
          }
          Builder.CreateCondBr(condition, ThenBB, MergeBB);
        }
      }
      Builder.SetInsertPoint(ThenBB);
      Value *ThenV = Codegen_Statement(TheFunction,statement->children[1]);
      Builder.CreateBr(MergeBB);
      //TheFunction->getBasicBlockList().push_back(ThenBB);
      ThenBB = Builder.GetInsertBlock();
      if(statement->children[2] != NULL){//ELSE is present
        TheFunction->getBasicBlockList().push_back(ElseBB);
        Builder.SetInsertPoint(ElseBB);
        Value *ElseV = Codegen_Statement(TheFunction, statement->children[2]);
        Builder.CreateBr(MergeBB);
      }
      TheFunction->getBasicBlockList().push_back(MergeBB);
      Builder.SetInsertPoint(MergeBB);
      return_value = MergeBB;
      scopeCount--;
    }
    
    else if(statement->sKind==project473::COMPOUND_STMT) {
      scopeCount++;
      if(statement->children[0] != NULL){
        for(project473::AstNodePtr tmpStmt = statement->children[0]; tmpStmt != NULL; tmpStmt = tmpStmt->sibling){
          return_value = Codegen_Statement(TheFunction, tmpStmt);
        }
      }else{
        return_value = NULL;
      }
      scopeCount--;
    }
    
    else if(statement->sKind==project473::WHILE_STMT) {
      scopeCount++;
      BasicBlock *WhileCond = BasicBlock::Create(getGlobalContext(), "while.cond", TheFunction);
      BasicBlock *WhileBody = BasicBlock::Create(getGlobalContext(), "while.body");
      BasicBlock *WhileEnd = BasicBlock::Create(getGlobalContext(), "while.end");
      Builder.CreateBr(WhileCond);
      Builder.SetInsertPoint(WhileCond);
      Value* condition = Codegen_Expression(statement->children[0]);
      if(dyn_cast<llvm::ConstantInt>(condition)){//condition is constant
        if(cast<llvm::ConstantInt>(condition)->isZero()){
          Builder.CreateBr(WhileEnd);
        }else{
          Builder.CreateBr(WhileBody);
        }
      }else{
          if(condition->getType() == Builder.getInt32Ty()){
            condition = Builder.CreateICmpNE(condition,Builder.getInt32(0),"tobool");
          }
          Builder.CreateCondBr(condition, WhileBody, WhileEnd);
      }
      TheFunction->getBasicBlockList().push_back(WhileBody);
      Builder.SetInsertPoint(WhileBody);
      Value *ThenV = Codegen_Statement(TheFunction,statement->children[1]);
      Builder.CreateBr(WhileCond);
      TheFunction->getBasicBlockList().push_back(WhileEnd);
      Builder.SetInsertPoint(WhileEnd);
      return_value = WhileEnd;
      scopeCount--;
    }
    return return_value; 

}

//generates the code for the body of the function. Steps
//1. Generate alloca instructions for the local variables
//2. Iterate over list of statements and call Codegen_Statement for each of them
Value* Codegen_Function_Body(Function *F, project473::AstNodePtr function_body) { 
  Value *return_value; 
  project473::ElementPtr declared_variable;
  char *name;
  llvm::IntegerType *intType = Builder.getInt32Ty();
  for(declared_variable = function_body->nSymbolTabPtr->queue; declared_variable != NULL; declared_variable = declared_variable->queue_next){
    name = strdup(declared_variable->id);
    if(NamedValues[name] == NULL){
      if(declared_variable->stype->kind==project473::INT){
        NamedValues[name] = Builder.CreateAlloca(intType, 0, name);
        NamedValues[name]->setAlignment(4);
      }else if(declared_variable->stype->kind==project473::ARRAY){
        llvm::ArrayType* arrType = llvm::ArrayType::get(intType,declared_variable->stype->dimension);
        NamedValues[name] = Builder.CreateAlloca(arrType, 0, name);
        NamedValues[name]->setAlignment(4);
      }
    }
  }
  project473::AstNodePtr tmp_stmt;
  for(tmp_stmt = function_body->children[0]; tmp_stmt != NULL; tmp_stmt = tmp_stmt->sibling){
    return_value =  Codegen_Statement(F, tmp_stmt);
  }

  return return_value;
}

//generates code for the function, verifies the code for the function. Steps:
//1. Create the entry block, place the insert point for the builder in the entry block
//2. Call CreateFormalVarAllocations
//3. Call Codegen_Function_Body
//4. Insert 'return' basic block if needed
Function* Codegen_Function(project473::AstNodePtr function_node) {
  std::string FunName(strdup(function_node->nSymbolPtr->id));
  Function *TheFunction = Functions[FunName];
  BasicBlock *EntryBB = BasicBlock::Create(getGlobalContext(), "entry", TheFunction);
  Builder.SetInsertPoint(EntryBB);

  CreateFormalVarAllocations(TheFunction,function_node);

  project473::AstNodePtr function_body = function_node->children[1]; //compound_stmt of the function is the body
  Codegen_Function_Body(TheFunction, function_body);

  if(ReturnBB != 0){
    Builder.SetInsertPoint(ReturnBB);
    if(TheFunction->getReturnType()->isIntegerTy()){
      Value *Val = Builder.CreateLoad(RetAlloca,"rval");
      Builder.CreateRet(Val);
    }else if(TheFunction->getReturnType()->isVoidTy()){
      Builder.CreateRetVoid();
    }
  }

  if(isReturnPresent == false){
    Builder.CreateRetVoid();
  }

  verifyFunction(*TheFunction);

  return TheFunction;
}

void initializeFunctions(project473::AstNodePtr program) {
    project473::AstNodePtr currentFun = program;
    while(currentFun != NULL) {
       Function *TheFunction = Codegen_Function_Declaration(currentFun);
       currentFun=currentFun->sibling;
    }
}

void codegen(char* filename) {
  InitializeNativeTarget();
  LLVMContext &Context = getGlobalContext();
  // Make the module, which holds all the code.
  TheModule = new Module(filename, Context);
  //Codegen the global variables
  project473::ElementPtr currentGlobal = symbolStackTop->symbolTablePtr->queue;

  while(currentGlobal != NULL) {
    if (currentGlobal->stype->kind==project473::INT) {
      llvm::IntegerType *intType = Builder.getInt32Ty();
      TheModule->getOrInsertGlobal(currentGlobal->id,intType);
      llvm::GlobalVariable* gVar = TheModule->getNamedGlobal(currentGlobal->id);
      gVar->setLinkage(llvm::GlobalValue::CommonLinkage);
      gVar->setAlignment(4);
      gVar->setInitializer(Builder.getInt32(0));
    }
    else if(currentGlobal->stype->kind==project473::ARRAY) {
      llvm::IntegerType *intType = Builder.getInt32Ty();
      int arraySize = currentGlobal->stype->dimension;
      llvm::ArrayType* arrType = llvm::ArrayType::get(intType,arraySize);
      TheModule->getOrInsertGlobal(currentGlobal->id,arrType);
      llvm::GlobalVariable* gVar = TheModule->getNamedGlobal(currentGlobal->id);
      gVar->setLinkage(llvm::GlobalValue::CommonLinkage);
      gVar->setAlignment(4);
      ConstantAggregateZero* array_initializer = ConstantAggregateZero::get(arrType);
      gVar->setInitializer(array_initializer);
    }
    currentGlobal = currentGlobal->queue_next;
  }
  
  initializeFunctions(program);
  project473::AstNodePtr currentFunction = program;

  while(currentFunction != NULL) {
    Function* theFunction = Codegen_Function(currentFunction); 
    currentFunction = currentFunction->sibling;
    isReturnPresent = false;
    NamedValues.clear();
    ReturnBB = 0;
    RetAlloca = 0;
  }
  // Print out all of the generated code.
  std::error_code ErrInfo;
  llvm::raw_ostream* filestream = new llvm::raw_fd_ostream("/home/dcasul3/hw6/codegen/out.s",ErrInfo, llvm::sys::fs::F_None);
  TheModule->print(*filestream,0);
}

int main(int argc, char** argv) {
  initLex(argc,argv);
  printf("lex done\n");
  yyparse();
  printf("parser done\n");
  int typ = typecheck();
  printf("typecheck returned: %d\n", typ);
  codegen(argv[1]);
 return 0;
}
