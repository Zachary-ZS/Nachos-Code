// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
//#include "synchconsole.h"
#include "addrspace.h"
#include "synch.h"

//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

void test_mul_thr(){
    printf("The second thread now starts!\n");
    machine->Run();
}

void
StartProcess(char *filename)
{
    /*
    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;
    OpenFile *executable2 = fileSystem->Open(filename);
    AddrSpace *space2;
    Thread *new_thr = new Thread ("test_mul_2");


    if (executable == NULL) {
	printf("Unable to open file %s\n", filename);
	return;
    }
    printf("initializing space for thread1\n");
    space = new AddrSpace(executable);
    printf("initializing space for thread2\n");
    space2 = new AddrSpace(executable2);    
    currentThread->space = space;
    space2->InitRegisters();
    space2->RestoreState();
    new_thr->space = space2;
    new_thr->Fork(test_mul_thr, 0);
    currentThread->Yield();

    delete executable;			// close file
    delete executable2;  

    space->InitRegisters();		// set the initial register values
    space->RestoreState();		// load page table register

    printf("Start the First thread's program\n");
    machine->Run();			// jump to the user progam
    ASSERT(FALSE);			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"

                    */
    // original code:
    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;

    if (executable == NULL) {
    printf("Unable to open file %s\n", filename);
    return;
    }
    space = new AddrSpace(executable);    
    currentThread->space = space;
    currentThread->filename = filename;

    delete executable;          // close file

    space->InitRegisters();     // set the initial register values
    space->RestoreState();      // load page table register

    machine->Run();         // jump to the user progam
    ASSERT(FALSE);          // machine->Run never returns;
                    // the address space exits
                    // by doing the syscall "exit"

}

// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.


//-------------------------------------------------------------------------------------------------
static Semaphore *readAvail = new Semaphore("readAvail", 0);
static Semaphore *writeDone = new Semaphore("writeDone", 0);


//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

/*void 
ConsoleTest (char *in, char *out)
{
    char ch;

    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    
    for (;;) {
	readAvail->P();		// wait for character to arrive
	ch = console->GetChar();
	console->PutChar(ch);	// echo it!
	writeDone->P() ;        // wait for write to finish
	if (ch == 'q') return;  // if q, quit
    }
}*/
class SynchConsole {
  public:
    SynchConsole(char *readfile, char *writefile);          // Initialize a synchronous console,
                    // by initializing the raw console.
    ~SynchConsole();
    
    void PutChar(char ch);
                    // display a single character.
    char GetChar();

  private:
    Console *console;
    Lock *lock;             // Only one read/write request
                    // can be sent to the console at a time   
};


static SynchConsole *synchconsole;

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

void ConsoleTest(char *in, char *out){
    char ch;
    synchconsole = new SynchConsole(in, out);
    for(;;){
        ch = synchconsole->GetChar();
        synchconsole->PutChar(ch);
        if(ch == 'q')
            return;
    }
}