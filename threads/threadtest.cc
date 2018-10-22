// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "elevatortest.h"
#include "thread.h"

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, (void*)1);
    SimpleThread(0);
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CheckTS(Thread *t){
	printf("---Thread Name:%s TID:%d Priority:%d Status:%s \n", t->getName(), t->getTID(), t->getPri(), t->getStatus());
}

void ThreadTest2(){
	DEBUG('t', "Entering ThreadTest2");

	printf("-----------------Test for Maximum number of Threads-------------\n");
	for(int i=0; i < 131; i++){
		Thread *thr = new Thread("Thread-test");
		thr->	Fork(CheckTS, (void *)thr);
	}
}

void CheckAllTS(){
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	
	List *Thread_list;
	Thread_list = scheduler->getReadyList(); 
	CheckTS(currentThread);//-------------------
	if(!Thread_list->IsEmpty())
		Thread_list->Mapcar(CheckTS);
	printf("\n\n");

    (void) interrupt->SetLevel(oldLevel);
}

void ThreadTest3(){
	DEBUG('t', "Entering ThreadTest2");

	printf("----------------------Check All Threads' Status-------------------\n");
	Thread *thr[3];
	for(int i=0; i < 3; i++)
		thr[i] = new Thread("Thread-test");
	for(int i=0; i < 3; i++){
		thr[i]->Fork(CheckAllTS, 0);
	}
	CheckAllTS();
	//printf("------------------------Check Ends--------------------------------\n");
}
// Test for Scheduler
void testfor4(){
	for(int i = 0; i < 3; i++){
		CheckTS(currentThread);
		if(i == 0){
			Thread *tnew = new Thread("Thread new", 5);
			tnew->Fork(CheckTS, (void *)tnew);
		}
			
	}	
}

void ThreadTest4(){
	Thread *t = new Thread("Thread 1", 10);
	t->Fork(testfor4, 0);
}
void testfor5(int num){
	for(int i = 0; i < num; i++){
		printf("---%s TID:%d Pri:%d looped for %d times.\n",currentThread->getName(), currentThread->getTID(), currentThread->getPri(), i);
		interrupt->SetLevel(IntOn);
		interrupt->SetLevel(IntOff);
	}
}
void ThreadTest5(){
	Thread *t1 = new Thread("Thr-1", 20);
	Thread *t2 = new Thread("Thr-2", 15);
	Thread *t3 = new Thread("Thr-3", 10);
	t1->Fork(testfor5, (void*)80);
	t2->Fork(testfor5, (void*)80);
	t3->Fork(testfor5, (void*)80);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
//-----------------------------------------------------------------------------
	case 2:
	ThreadTest2();
	break;
	case 3:
	ThreadTest3();
	break;
	case 4:
	ThreadTest4();
	break;
	case 5:
	ThreadTest5();
	break;
 //-----------------------------------------------------------------------------
   default:
	printf("No test specified.\n");
	break;
    }
}

