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
#include "openfile.h"
#include "filesys.h"
#include "directory.h"

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
// ------------------LRU for pagetable------------------------------------------------------
int LRU_pt(){
    //    Here we need an array to store the state of each tlb plot
    // which is lut, and we find the least recnetly used plot.
    int index = -1;
    int maxt = 0;
    for(int i = 0; i < NumPhysPages; i++){
        //printf("%d\n", machine->LUtime[i]);
        if(machine->lut[i] > maxt){
            index = i;
            maxt = machine->lut[i];
        }
        //else if(machine->LUtime[i] != 0)
        //  machine->LUtime[i]++;
    }
    ASSERT(index != -1);
    return index;
}
//lab6----------------------------------------------------
void execute(int addr){
    // execute a user program
    char name[16];
    int index = 0;
    int ch;

    while(TRUE){
        machine->ReadMem(addr+index, 1, &ch);
        if(ch == 0){
            if(index != 0){
            name[index] = '\0';
            break;
            }
        }
        name[index++] = char(ch);
        //printf("%c\n", char(ch));
    }
    name[0] = '.';
    printf("A new thr running %s.\n", name);
    OpenFile *executable = fileSystem->Open(name);
    AddrSpace *space;
    space = new AddrSpace(executable);
    currentThread->space = space;
    delete executable;
    space->InitRegisters();
    space->RestoreState();
    machine->Run();
}
struct Info{
    AddrSpace *space;
    int PC;
};

void forker(int addr){
    Info *info = (Info*)addr;
    AddrSpace *space = info->space;
    currentThread->space = space;
    int pc = info->PC;
    space->InitRegisters();
    space->RestoreState();
    machine->WriteRegister(PCReg, pc);
    machine->WriteRegister(NextPCReg, pc + 4);
    machine->Run();
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
    	// tlb miss or pagetable miss?-------------------------------------------------------------------
		if(machine->pageTable != NULL){
			
    		OpenFile *ofile = fileSystem->Open("virtual_memory");
            ASSERT(ofile != NULL);
            int vpn = (unsigned) machine->ReadRegister(BadVAddrReg) / PageSize;
            int index = machine->pagemap->Find();
            if(index == -1){
                // no empty physical page, needs to kick out one
                // 

                /* here we use a quite naive algorithm, we kick out the first phypage.
                for(int j = 0; j < machine->pageTableSize; j++){
                    if(machine->pageTable[j].physicalPage == 0){
                        // dirty -> we should write back
                        if(machine -> pageTable[j].dirty == TRUE){
                            ofile -> WriteAt(&(machine->mainMemory[index * PageSize]), PageSize, machine->pageTable[j].virtualPage * PageSize);
                            machine -> pageTable[j].valid = FALSE;
                            break;
                        }
                    }
                }
                index = 0;*/
                index = LRU_pt();
                if(machine -> pageTable[index].dirty == TRUE){
                    ofile -> WriteAt(&(machine->mainMemory[index * PageSize]), PageSize, machine->pageTable[index].virtualPage * PageSize);
                    //machine -> pageTable[index].valid = FALSE;
                }
            }
            //printf("--Caused PageFault! Loading page-vpn:%d into pagetable [%d] for thr-%d\n", vpn, index, currentThread->getTID());
            ofile->ReadAt(&(machine->mainMemory[index * PageSize]), PageSize, vpn * PageSize);
            machine->pageTable[index].valid = TRUE;
            machine->pageTable[index].virtualPage = vpn;
            machine->pageTable[index].tid = currentThread->getTID();
            machine->pageTable[index].use = FALSE;
            machine->pageTable[index].dirty = FALSE;
            machine->pageTable[index].readOnly = FALSE;
            
            delete ofile;
            
		}
    	else{   // tlb miss, VA causing exception is now in BadVAreg
    		int vpn = (unsigned) machine->ReadRegister(BadVAddrReg) / PageSize;

    		ASSERT(vpn < machine->pageTableSize);
    		//ASSERT(machine->pageTable[vpn].valid);

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
    //-----------------------------------------------------------------------------------------------------------------------
    else if(which == SyscallException && type == SC_Exit){
        printf("The program exits...\n");
        machine->clearpage();
        currentThread->Finish();
        int nextPC = machine->ReadRegister(NextPCReg);
        machine->WriteRegister(PCReg, nextPC);
    }
    // lab6
    else if(which == SyscallException && type == SC_Create){
        printf("Syscall: Create a file.\n");
        int addr = machine->ReadRegister(4);
        char name[FileNameMaxLen + 1];
        int index = 0, ch;
        while(TRUE){
            machine->ReadMem(addr + index, 1, &ch);
            if(ch == 0){
                name[index] = '\0';
                break;
            }
            //printf("%c\n", char(ch));
            name[index++] = char(ch);
        }
        name[0] = 'n';
        printf("Creating a file named %s\n", name);
        fileSystem->Create(name, SectorSize);
        machine->PCplus();

    }
    else if(which == SyscallException && type == SC_Open){
        printf("Syscall: Open a file.\n");
        int addr = machine->ReadRegister(4);
        char name[FileNameMaxLen + 1];
        int index = 0, ch = 0;
        while(TRUE){
            machine->ReadMem(addr + index, 1, &ch);
            if(ch == 0){
                name[index] = '\0';
                break;
            }
            name[index++] = char(ch);
        }
        printf("Opening file %s.\n", name);
        OpenFile *openfile = fileSystem->Open(name);
        if(openfile == NULL){
            printf("Failed! file not found.\n");
            machine->WriteRegister(2, 0);
        }
        else{
            machine->WriteRegister(2, int(openfile));
            printf("Opened the file with opfileID %d.\n", int(openfile));
        }
        machine->PCplus();
        //printf("???\n");
    }
    else if(which == SyscallException && type == SC_Close){
        printf("Syscall: Close a file.\n");
        int fid = machine->ReadRegister(4); 
        OpenFile *openfile = (OpenFile*)fid;
        delete openfile;
        machine->PCplus();
    }
    else if(which == SyscallException && type == SC_Read){
        printf("Syscall: Read a file.\n");
        int position = machine->ReadRegister(4);
        int size = machine->ReadRegister(5);
        int fid = machine->ReadRegister(6);

        OpenFile *openfile = (OpenFile*)fid;
        if(openfile == NULL){
            printf("File not exist!\n");
        }
        else{
            char *tmp = new char[size + 1];
            int actualsize = openfile->Read(tmp, size);
            tmp[actualsize] = '\0';
            for(int i = 0; i < actualsize; i++)
                machine->WriteMem(position + i, 1,int(tmp[i]));
            machine->WriteRegister(2, actualsize);
        }
        machine->PCplus();
    }
    else if(which == SyscallException && type == SC_Write){
        printf("Syscall: Write a file.\n");
        int position = machine->ReadRegister(4);
        int size = machine->ReadRegister(5);
        int fid = machine->ReadRegister(6);

        char *tmp = new char[size + 1];
        int ch;
        for(int i = 0; i < size; i++){
            machine->ReadMem(position + i, 1, &ch);
            tmp[i] = char(ch);
        }

        OpenFile *openfile = (OpenFile*)fid;
        if(openfile == NULL){
            printf("File not exist!\n");
        }
        else{
            openfile->Write(tmp, size);
        }
        machine->PCplus();
    }
    else if(which == SyscallException && type == SC_Exec){
        printf("Syscall: Exec a user prog.\n");
        int addr = machine->ReadRegister(4);
        Thread *newthr = new Thread("thr-side");
        newthr->Fork(execute, addr);
        machine->WriteRegister(2, newthr->getTID());
        machine->PCplus();
    }
    else if(which == SyscallException && type == SC_Fork){
        printf("Syscall: Fork.\n");
        int funcaddr = machine->ReadRegister(4);
        /*
        OpenFile *executable = fileSystem->Open(currentThread->filename);
        AddrSpace *space = new AddrSpace(executable);

        */
        AddrSpace *space = currentThread->space;
        // A new structure  to transmit args to fork
        Info *info = new Info;
        info->space = space;
        info->PC = funcaddr;
        Thread *newthr = new Thread("thr-son");
        newthr->Fork(forker, info);
        machine->WriteRegister(2, newthr->getTID());
        machine->PCplus();
    }
    else if(which == SyscallException && type == SC_Yield){
        printf("Syscall: Yield.\n");
        machine->PCplus();
        currentThread->Yield();
    }
    else if(which == SyscallException && type == SC_Join){
        printf("Syscall: Join.\n");
        int tid = machine->ReadRegister(4);
        while(tid_used[tid]){
            printf("Son-thread hasn't exited.\n");
            currentThread->Yield();
        }
        machine->PCplus();
    }
    


    else {
    	printf("Unexpected user mode exception %d %d\n", which, type);
    	ASSERT(FALSE);
    }
}
