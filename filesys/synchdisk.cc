// synchdisk.cc 
//	Routines to synchronously access the disk.  The physical disk 
//	is an asynchronous device (disk requests return immediately, and
//	an interrupt happens later on).  This is a layer on top of
//	the disk providing a synchronous interface (requests wait until
//	the request completes).
//
//	Use a semaphore to synchronize the interrupt handlers with the
//	pending requests.  And, because the physical disk can only
//	handle one operation at a time, use a lock to enforce mutual
//	exclusion.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synchdisk.h"
#include "stats.h"
#include "system.h"

//----------------------------------------------------------------------
// DiskRequestDone
// 	Disk interrupt handler.  Need this to be a C routine, because 
//	C++ can't handle pointers to member functions.
//----------------------------------------------------------------------

static void
DiskRequestDone (int arg)
{
    SynchDisk* disk = (SynchDisk *)arg;

    disk->RequestDone();
}

//----------------------------------------------------------------------
// SynchDisk::SynchDisk
// 	Initialize the synchronous interface to the physical disk, in turn
//	initializing the physical disk.
//
//	"name" -- UNIX file name to be used as storage for the disk data
//	   (usually, "DISK")
//----------------------------------------------------------------------

SynchDisk::SynchDisk(char* name)
{
    semaphore = new Semaphore("synch disk", 0);
    lock = new Lock("synch disk lock");
    disk = new Disk(name, DiskRequestDone, (int) this);
    for (int i = 0; i < NumSectors; i++){
        mutex[i] = new Semaphore("mutexforfile", 1);
        numReaders[i] = 0;
        numVisitors[i] = 0;
    }
    for (int i = 0; i < cachesize; i++){
        cache[i].valid = FALSE;
    }
    readerlock = new Lock("reader lock");
}

//----------------------------------------------------------------------
// SynchDisk::~SynchDisk
// 	De-allocate data structures needed for the synchronous disk
//	abstraction.
//----------------------------------------------------------------------

SynchDisk::~SynchDisk()
{
    delete disk;
    delete lock;
    delete semaphore;
}

//----------------------------------------------------------------------
// SynchDisk::ReadSector
// 	Read the contents of a disk sector into a buffer.  Return only
//	after the data has been read.
//
//	"sectorNumber" -- the disk sector to read
//	"data" -- the buffer to hold the contents of the disk sector
//----------------------------------------------------------------------

void
SynchDisk::ReadSector(int sectorNumber, char* data)
{
    int pos = -1;
    for(int i = 0; i < cachesize; i++)
        if(cache[i].valid && cache[i].sector == sectorNumber){
            pos = i;
            break;
        }
    if(pos != -1){
        printf("cache for disk Hit!\n");
        cache[pos].lrutime = stats->totalTicks;
        bcopy(cache[pos].data, data, SectorSize);
        //disk->HandleInterrupt();
    }
    else{
        lock->Acquire();            // only one disk I/O at a time
        disk->ReadRequest(sectorNumber, data);
        semaphore->P();         // wait for interrupt
        lock->Release();

        int index = -1;
        for(int i = 0; i < cachesize; i++){
            if(!cache[i].valid){
                index = i;
                break;
            }
        }
        if(index == -1){
            int minT = cache[0].lrutime, minindex = 0;
            for(int i = 1; i < cachesize; i++){
                if(cache[i].lrutime < minT){
                    minT = cache[i].lrutime;
                    minindex = i;
                }
            }
            index = minindex;
        }
        cache[index].valid = TRUE;
        cache[index].dirty = FALSE;
        cache[index].sector = sectorNumber;
        cache[index].lrutime = stats->totalTicks;
        bcopy(data, cache[index].data, SectorSize);
    }



    
}

//----------------------------------------------------------------------
// SynchDisk::WriteSector
// 	Write the contents of a buffer into a disk sector.  Return only
//	after the data has been written.
//
//	"sectorNumber" -- the disk sector to be written
//	"data" -- the new contents of the disk sector
//----------------------------------------------------------------------

void
SynchDisk::WriteSector(int sectorNumber, char* data)
{
    lock->Acquire();			// only one disk I/O at a time
    disk->WriteRequest(sectorNumber, data);
    semaphore->P();			// wait for interrupt
    lock->Release();
    for(int i = 0; i < cachesize; i++)
        if(cache[i].sector == sectorNumber)
            cache[i].valid = FALSE;
}

//----------------------------------------------------------------------
// SynchDisk::RequestDone
// 	Disk interrupt handler.  Wake up any thread waiting for the disk
//	request to finish.
//----------------------------------------------------------------------

void
SynchDisk::RequestDone()
{ 
    semaphore->V();
}

void SynchDisk::readerplus(int sector){
    readerlock->Acquire();
    numReaders[sector]++;
    if(numReaders[sector] == 1)
        mutex[sector]->P();
    printf("A new reader. Now the total num of readers: %d\n", numReaders[sector]);
    readerlock->Release();
}
void SynchDisk::readerminus(int sector){
    readerlock->Acquire();
    numReaders[sector]--;
    if(numReaders[sector] == 0)
        mutex[sector]->V();
    printf("---A reader left. The num of readers now is: %d\n", numReaders[sector]);
    readerlock->Release();
}
void SynchDisk::startwriting(int sector){
    mutex[sector]->P();
}
void SynchDisk::endwriting(int sector){
    mutex[sector]->V();
}
