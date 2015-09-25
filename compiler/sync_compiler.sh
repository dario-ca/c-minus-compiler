#!/bin/bash
scp -r codegen compiler:~/cminus-compiler
#next line works because we commented the interactive flag at the beginning of .bashrc file
#in this way the script is run even if we are sending commands remotely
ssh compiler 'cd ~/hw6/codegen/;
				make clean;
				make;
				./codegen test.c;
				echo "======== My IR-CODE ===========================";
				clang -S -emit-llvm -o out_clang.s test.c;
				cat out.s;
				echo "======== Clang IR-CODE ========================";
				cat out_clang.s;'
				

#to have the complete executable:

#llvm-as test.ll -o test.bc
#llc test.bc -o test.s
#clang test.s -o test