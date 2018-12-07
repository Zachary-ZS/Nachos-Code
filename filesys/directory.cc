// directory.cc 
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size)
{
    table = new DirectoryEntry[size];
    tableSize = size;
    for (int i = 0; i < tableSize; i++)
	table[i].inUse = FALSE;
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{ 
    delete [] table;
} 

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void
Directory::FetchFrom(OpenFile *file)
{
                    //printf("Writin back to sector in FetchFrom !!!!\n");
                    //file->gethdr()->Print();
                    //printf("----------------------\n");
    (void) file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
    //printf("Fetching>???\n");
    //Print();
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void
Directory::WriteBack(OpenFile *file)
{
    (void) file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::FindIndex(char *name)
{
    //printf("adqwdqw, %d\n", tableSize);
    for (int i = 0; i < tableSize; i++) {
        //printf("-----%s\n", table[i].name);
        if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))
    	    return i;
}
    return -1;		// name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't 
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::Find(char *name)
{
    int i = FindIndex(name);

    if (i != -1)
	return table[i].sector;
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------
// ty means the type of the directory-entry's file type: 1 means a directory file, 0 for normal
bool
Directory::Add(char *name, int newSector, bool ty = false)
{ 
    char fname[FileNameMaxLen + 1];
    int pi = -1;
            //printf("???%s\n", name);
    for(int i = strlen(name) - 1; i >= 0; i--)
        if(name[i] == '/'){
            pi = i+1;
            break;
        }
    if(pi == -1)
        pi = 0;
    int j = 0;
    for(int i = pi; i < strlen(name); i++)
        fname[j++] = name[i];
    fname[j] = '\0';

    if (FindIndex(fname) != -1)
	return FALSE;

    char *trew[2]={"normal","directory"};
    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse) {
            table[i].inUse = TRUE;
            strncpy(table[i].path, name, 20);
            strncpy(table[i].name, fname, FileNameMaxLen); 
            table[i].sector = newSector;
            table[i].type = ty;
            printf("file %s added to directory. It's a %s file.\n", name, trew[ty]);
            List();
        return TRUE;
	}
    return FALSE;	// no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory. 
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool
Directory::Remove(char *name)
{ 
    int i = FindIndex(name);

    printf("Deleting file %s in table[%d]---\n", name, i);
    if (i == -1)
	return FALSE; 		// name not in directory
    table[i].inUse = FALSE;
    return TRUE;	
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory. 
//----------------------------------------------------------------------

void
Directory::List()
{
   for (int i = 0; i < tableSize; i++)
	if (table[i].inUse)
	    printf("%s\n", table[i].name);
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void
Directory::Print()
{ 
    FileHeader *hdr = new FileHeader;

    char *filetypechar[] = {"normal file", "directory-file"};

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++)
	if (table[i].inUse) {
	    printf("Name: %s, Sector: %d, Type: %s\n", table[i].name, table[i].sector, filetypechar[table[i].type]);
	    hdr->FetchFrom(table[i].sector);
	    hdr->Print();
        // if(table[i].type){
        //     // It's a dir, let's list its files.
        //     FileHeader *newhdr = new FileHeader;
        //     newhdr.FetchFrom(table[i].sector);
        //     printf("")
        //     newhdr->Print();

        // }
	}
    printf("\n");
    delete hdr;
}

int
Directory::findsector(char *name){
                   // printf("Writin back to sector !!!!\n");
    int len = strlen(name);
    int sec = 1;
    // Search for it from root.
    OpenFile *dirf = new OpenFile(sec);
                   // printf("Writin back to sector !!!!\n");
    Directory *dir = new Directory(10); // no need to assign table
                   // printf("Writin back to sector !!!!\n");
    dir->FetchFrom(dirf);
    int pi = 0;
    int tmpi = 0;
    char tmp[10];
                   // printf("Writin back to sector !!!!\n");
    while(pi < len){
        tmp[tmpi++] = name[pi++];
        if(name[pi]=='/'){
            tmp[tmpi] = '\0';
            printf("Now we're getting into dir---%s-------\n", tmp);
            sec = dir->Find(tmp);
            //printf("------into %s %d\n", tmp, sec);
            dirf = new OpenFile(sec);
            //printf("------into %s\n", tmp);
            dir = new Directory(10);
            dir->FetchFrom(dirf);
            pi++; tmpi = 0;
        }
    }
    return sec;
}

bool Directory::isdir(char *name){
    int index = FindIndex(name);
    return table[index].type;
}

bool Directory::isempty(){
    for (int i = 0; i < tableSize; i++)
        if(table[i].inUse)
            return FALSE;
    return TRUE;
}