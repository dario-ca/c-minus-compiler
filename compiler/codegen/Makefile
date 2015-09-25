# Makefile for cminus

CC=gcc
CPP=clang++
LEX=flex
YACC=bison
MV=/bin/mv
RM=/bin/rm -rf
CP=/bin/cp

INCLUDES=/llvm/llvm/include/

CFLAGS=-g
YFLAGS=-dv

PROGRAM=codegen

OBJECTS=codegen.o cmlexer.o cmparser.o symbolTable.o ast.o util.o typecheck.o	

SOURCES=cmlexer.l cmparser.y symbolTable.c ast.c util.c typecheck.c

CTEMPS=cmlexer.c cmparser.c cmparser.tab.h cmparser.tab.c cmparser.output

$(PROGRAM): $(OBJECTS)
	$(CPP) `llvm-config --ldflags` $(CFLAGS) -o codegen $(OBJECTS) `llvm-config --libs` -ll -ldl -pthread -lpthread

typecheck.o: typecheck.c
	$(CC) $(CFLAGS) -c typecheck.c

util.o: util.c
	$(CC) $(CFLAGS) -c util.c

ast.o:  ast.c
	$(CC) $(CFLAGS) -c ast.c

symbolTable.o: symbolTable.c
	$(CC) $(CFLAGS) -c symbolTable.c

cmparser.o: cmparser.c 
	$(CC) $(CFLAGS) -c cmparser.c

cmparser.c: cmparser.y
	$(YACC)  cmparser.y -o cmparser.c

cmlexer.o: cmlexer.c cmparser.tab.h globals.h
	$(CC) $(CFLAGS) -c cmlexer.c

cmparser.tab.h: cmparser.y
	$(YACC) $(YFLAGS)  cmparser.y 

cmlexer.c: cmlexer.l 
	$(LEX) -ocmlexer.c  cmlexer.l 

codegen.o: codegen.cpp
	$(CPP) `llvm-config --cxxflags` $(CFLAGS) -o codegen.o -c codegen.cpp

clean:
	$(RM) $(PROGRAM) $(OBJECTS) $(CTEMPS)
