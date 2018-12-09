// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"
#include <time.h>

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    set_createtime();
    set_lastvitime();
    set_lastmodtime();

    if (freeMap->NumClear() < numSectors + 1)
	return FALSE;		// not enough space
    //------------------------------------------------------------------------------------------------
    if(numSectors < NumDirect){
        // why not '='? cuz we need a sector to store indrect-index
        // In fact, now the NumDirect is only 11 which means that 
        // most files would use indrect-index.
        for (int i = 0; i < numSectors; i++)
            dataSectors[i] = freeMap->Find();
    }
    else{
        for(int i = 0; i < NumDirect; i++)
            dataSectors[i] = freeMap->Find();
        int indrect_index[32] = {0};
        for(int i = 0; i < numSectors - NumDirect + 1; i++)
            indrect_index[i] = freeMap->Find();
        synchDisk->WriteSector(dataSectors[NumDirect - 1], (char *)indrect_index);
    }

    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    if(numSectors < NumDirect){
        for(int i = 0; i < numSectors; i++){
            ASSERT(freeMap->Test((int) dataSectors[i]));
            freeMap->Clear((int) dataSectors[i]);
        }
    }
    else{
        char *ind = new char[SectorSize];
        synchDisk->ReadSector(dataSectors[NumDirect - 1], ind);
        for(int i = 0; i < NumDirect; i++)
            freeMap->Clear((int)dataSectors[i]);
        for(int i = 0; i < numSectors - NumDirect + 1; i++)
            freeMap->Clear((int)ind[i*4]);
    }

 //    for (int i = 0; i < numSectors; i++) {
	// ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
	// freeMap->Clear((int) dataSectors[i]);
 //    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    if(numSectors < NumDirect)
        return(dataSectors[offset / SectorSize]);
    else{
        if(offset < (NumDirect - 1)*SectorSize)
            return (dataSectors[offset / SectorSize]);
        char *ind = new char[SectorSize];
        synchDisk->ReadSector(dataSectors[NumDirect - 1], ind);
        int sec = (offset - (NumDirect - 1)*SectorSize) / SectorSize;
        return ((int)ind[sec * 4]);
    }
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.\n", numBytes);
    printf("\tFile created at the time %s,\n\tlast visited at %s,  last modified at %s.\n  File blocks:\n", createtime, lastvitime, lastmodtime);
    
    if(numSectors < NumDirect){
        for (i = 0; i < numSectors; i++)
	       printf("%d ", dataSectors[i]);
    }
    else{
        printf("File is using indrect_index, which is stored in sector %d.  Here are all the physical sectors used:\n", dataSectors[NumDirect - 1]);
        for (i = 0; i < NumDirect - 1; i++)
           printf("%d ", dataSectors[i]);
        char *ind = new char[SectorSize];
        synchDisk->ReadSector(dataSectors[NumDirect - 1], ind);
        for(i = 0; i < numSectors - NumDirect + 1; i++){
            printf("%d ",int(ind[i*4]));
        }
    }
    printf("\nFile contents:\n");
    if(numSectors < NumDirect){
    for (i = k = 0; i < numSectors; i++) {
        synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	        if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		        printf("%c", data[j]);
            else
        		printf("\\%x", (unsigned char)data[j]);
	    }
        printf("\n"); 
    }
    }
    else{
        for (i = k = 0; i < NumDirect - 1; i++) {
            synchDisk->ReadSector(dataSectors[i], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
                if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
                    printf("%c", data[j]);
                else
                    printf("\\%x", (unsigned char)data[j]);
            }
            printf("\n"); 
        }
        char *ind = new char[SectorSize];
        synchDisk->ReadSector(dataSectors[NumDirect - 1], ind);
        for(i = 0; i < numSectors - NumDirect + 1; i++){
            synchDisk->ReadSector((int)ind[i*4], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
                if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
                    printf("%c", data[j]);
                else
                    printf("\\%x", (unsigned char)data[j]);
            }
            printf("\n"); 
        }
    }

    delete [] data;
}
void FileHeader::set_createtime(){
    time_t currentT;
    time(&currentT);
    strncpy(createtime, asctime(gmtime(&currentT)), 25);
    createtime[24] = '\0';
    printf("Filed created at the time %s.\n", createtime);
}

void FileHeader::set_lastvitime(){
    time_t currentT;
    time(&currentT);
    strncpy(lastvitime, asctime(gmtime(&currentT)), 25);
    lastvitime[24] = '\0';
    //printf("Filed visited at the time %s.\n", lastvitime);
}

void FileHeader::set_lastmodtime(){
    time_t currentT;
    time(&currentT);
    strncpy(lastmodtime, asctime(gmtime(&currentT)), 25);
    lastmodtime[24] = '\0';
    //printf("Filed modified at the time %s.\n", lastmodtime);
}

void FileHeader::Extend(BitMap *freeMap, int bytes){
    if(bytes <= 0)
        return TRUE;
    // extend by "bytes" bytes
    numBytes = numBytes + bytes;
    int origin_sector = numSectors;
    numSectors = divRoundUp(numBytes, SectorSize);
    if(origin_sector == numSectors)
        return TRUE;    // nothing todo cuz the rest of the last sector is enough
    if(freeMap->NumClear() < numSectors - origin_sector){
        printf("Sorry! There's No enough space to extend.\n");
        return FALSE;
    }
    // direct index still
    if(numSectors < NumDirect)
        for(int i = origin_sector; i < numSectors; i++)
            dataSectors[i] = freeMap->Find();
    else if(origin_sector >= NumDirect){
        // benlaijiushi using indirect_index
        int ind[32] = {0};
        synchDisk->ReadSector(dataSectors[NumDirect - 1], (char*)ind);

        for(int i = 0; i < numSectors - origin_sector; i++)
            ind[origin_sector - NumDirect + 1 + i] = freeMap->Find();
        synchDisk->WriteSector(dataSectors[NumDirect - 1], (char *)ind);
    }
    else{
        // benlai meiyou jianjie ,we need to add indirect:
        for(int i = origin_sector; i < NumDirect; i++)
            dataSectors[i] = freeMap->Find();
        int indrect_index[32] = {0};
        for(int i = 0; i < numSectors - NumDirect + 1; i++)
            indrect_index[i] = freeMap->Find();
        synchDisk->WriteSector(dataSectors[NumDirect - 1], (char *)indrect_index);
        printf("We added an indirect index for the file.\n");
    }
    printf("Extended the file with %d more sectors.\n", numSectors - origin_sector);
    return TRUE;
}
