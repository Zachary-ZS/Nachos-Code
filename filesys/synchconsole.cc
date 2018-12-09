#include "copyright.h"
#include "synchconsole.h"


SynchConsole::SynchConsole(char *readFile, char *writeFile){

	console = new Console(readFile, writeFile, ReadAvail, WriteDone, 0);
	lock = new Lock("console");
}
void SynchConsole::PutChar(char ch){
	lock->Acquire();
	console->PutChar(ch);
	writeDone->P();
	lock->Release();
}
char SynchConsole::GetChar(){
	lock->Acquire();
	readAvail->P();
	char ch = console->GetChar();
	lock->Release();
	return ch;
}
