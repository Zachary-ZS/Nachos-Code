/*#include "syscall.h"

int fid1, fid2;
char buf[22];
int actsize;

int main(){

	Create("new.txt");
	fid1 = Open("mysmall.txt");
	actsize = Read(buf, 21, fid1);
	fid2 = Open("new.txt");
	Write(buf, actsize, fid2);
	Close(fid1);
	Close(fid2);
	Halt();

}
*/

#include "syscall.h"

int main(){

	int fid = Exec("../test/halt");
	Join(fid);
	Create("new.txt");
}


/*
#include "syscall.h"
int fid1;
void createfile(){
	Create("new1.txt");
	Exit(0);
}

int main(){

	Create("new.txt");
	Fork(createfile);
	Create("new2.txt");
}
*/