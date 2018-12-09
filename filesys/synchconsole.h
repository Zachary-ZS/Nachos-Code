// synchconsole.h 
// 	Data structures to export a synchronous interface to the raw 
//	console device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.
// Copyright @ zhangsu: This file is created and modified by zhangsu.
// Written based on the "synchdisk.h" file.

#include "copyright.h"

//#ifndef SYNCHDISK_H
//#define SYNCHDISK_H

#include "console.h"
#include "synch.h"

// The following class defines a "synchronous" console .
// As with other I/O devices, the raw console is an asynchronous device --
// 
// This class provides the abstraction that for any individual thread
// making a request, it waits around until the operation finishes before
// returning.

static Semaphore *readAvail = new Semaphore("readAvail", 0);
static Semaphore *writeDone = new Semaphore("writeDone", 0);
static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

class SynchConsole {
  public:
    SynchConsole(char *readfile, char *writefile);    		// Initialize a synchronous console,
					// by initializing the raw console.
    ~SynchConsole();
    
    void PutChar(char ch);
                    // display a single character.
    char GetChar();

  private:
    Console *console;
    Lock *lock;		  		// Only one read/write request
					// can be sent to the console at a time   
};

//#endif // SYNCHDISK_H
