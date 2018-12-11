#include "syscall.h"
#define M 400
#define N 400
int num[M][N];
int main(){
	int i = 0, j = 0;
	for(i = 0; i < M; i++)
		for(j = 0; j < N; j++)
			num[i][j] = 0;
	//Halt();
}