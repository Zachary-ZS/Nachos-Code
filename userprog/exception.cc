// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------


// -----------------------------------------------------------------------------------------
// Replacement algorithms:
int FIFO(){
	for(int i = 0; i < TLBSize - 1; i++)
		machine->tlb[i] = machine->tlb[i + 1];
	return TLBSize - 1;
}
int LRU(){
	//    Here we need an array to store the state of each tlb plot
	// which is LUtime, and we find the least recnetly used plot.
	int index = -1;
    int maxt = 0;
	for(int i = 0; i < TLBSize; i++){
		//printf("%d\n", machine->LUtime[i]);
        if(machine->LUtime[i] > maxt){
			index = i;
            maxt = machine->LUtime[i];
		}
		//else if(machine->LUtime[i] != 0)
		//	machine->LUtime[i]++;
	}
	ASSERT(index != -1);
	return index;
}
void
ExceptionHandler(ExceptionType which)
{
	int type = machine->ReadRegister(2);

	if ((which == SyscallException) && (type == SC_Halt)) {
		DEBUG('a', "Shutdown, initiated by user program.\n");
		interrupt->Halt();
	}
	else if (which == PageFaultException){
    	// tlb miss or pagetable miss?
		if(machine->tlb == NULL){
			ASSERT(FALSE);
    		// shouldn't miss in pagetable
		}
    	else{   // tlb miss, VA causing exception is now in BadVAreg
    		int vpn = (unsigned) machine->ReadRegister(BadVAddrReg) / PageSize;

    		ASSERT(vpn < machine->pageTableSize);
    		ASSERT(machine->pageTable[vpn].valid);

    		int index = -1;
    		for(int i = 0; i < TLBSize; i++){
    			if(!machine->tlb[i].valid){
    				index = i;
    				break;
    			}
    		}
    		// No empty plot in tlb
    		if(index == -1){
    			// Repalcement Algorithm: now support FIFO & LRU, choose only one while using
    			//index = FIFO(); // FIFO
    			index = LRU();  // LRU

    		}
    		// load the page into the tlb
    		machine->tlb[index].valid = true;
    		machine->tlb[index].use = false;
    		machine->tlb[index].dirty = false;
    		machine->tlb[index].virtualPage = vpn;
    		machine->tlb[index].physicalPage = machine->pageTable[vpn].physicalPage;
    		machine->tlb[index].readOnly = false;
    	}
    	
    }
    else if(which == SyscallException && type == SC_Exit){
        printf("The program exits...\n");
        machine->clearpage();
        currentThread->Finish();
        int nextPC = machine->ReadRegister(NextPCReg);
        machine->WriteRegister(PCReg, nextPC);
    }
    else {
    	printf("Unexpected user mode exception %d %d\n", which, type);
    	ASSERT(FALSE);
    }
}
