int g1[100];
int g2;

void foo(void){
	int x;
	int y[10];
	return;
}

int foo2(int a, int b[]){
	a=2;
	a=g2;
	b[1]=2;
	b[2]=g2;
	b[3]=b[4];
	return a;
}

int foo3(void){
	int x;
	x = foo2(g2,g1);
	return 0;
}

int ifstmt(void){
	if(g2==0){
		return 0;
	}else{
		return 1;
	}
	return 2;
}

int whilestmt(void){
	int a;
	a=g2;
	while(g2>a){
		a=g2;
	}
	return a;
}