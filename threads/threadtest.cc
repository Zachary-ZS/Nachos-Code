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
#include "synch.h"

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
//--------The following is for synchronization lab-----------------------------
//-----------------------------------------------------------------------------
List *buffer;
#define maxnum_buf 5
//-----------------------------------------------------------------------------
// Implementation of Producer&Consumer with Lock & Condition.------------------
Lock *mutex_buf;
Condition *cond_buf;
void producer1(int num = 1){
	
	mutex_buf->Acquire();
	while(num--){
		//int *tmp = 1;
		if(buffer->NumInList() == maxnum_buf){
			printf("Sorry! No more empty slots to produce in.\n");
			cond_buf->Wait(mutex_buf);
		}
		buffer->Append((void*)1);
		printf("----------%s Produced a new item, now the number of items is %d.\n", currentThread->getName(), buffer->NumInList());
		if(buffer->NumInList() == 1){
			cond_buf->Signal(mutex_buf);
		}
		//printf("Finished.\n");
	}
	printf("Finished Producing.\n");
	mutex_buf->Release();
}
void consumer1(int num = 1){

	mutex_buf->Acquire();
	while(num--){
		if(buffer->NumInList() == 0){
			printf("Sorry! The buffer is now empty!\n");
			cond_buf->Wait(mutex_buf);
		}
		buffer->Remove();
		printf("----------%s Consumed an item, now the number of items is %d.\n", currentThread->getName(), buffer->NumInList());
		if(buffer->NumInList() == maxnum_buf - 1){
			cond_buf->Signal(mutex_buf);
		}
	}
	printf("Finished Consuming.\n");
	mutex_buf->Release();
}

void ThreadTest6(){

	buffer = new List();
	mutex_buf = new Lock("Mutex_Buffer");
	cond_buf = new Condition("Condition_Buffer");
//---------------------------------------------------------
// Easy Test or Complex Test?

/*
	Thread *t1 = new Thread("Thr-Producer");
	Thread *t2 = new Thread("Thr-Consumer");

	t1->Fork(producer1, (void*)8);
	t2->Fork(consumer1, (void*)10);
*/
	Thread *p1 = new Thread("Thr-Producer1");
	Thread *p2 = new Thread("Thr-Producer2");
	Thread *p3 = new Thread("Thr-Producer3");
	Thread *c1 = new Thread("Thr-Consumer1");
	Thread *c2 = new Thread("Thr-Consumer2");
	Thread *c3 = new Thread("Thr-Consumer3");

	p1->Fork(producer1, (void*)4);
	c1->Fork(consumer1, (void*)6);
	p2->Fork(producer1, (void*)5);
	c2->Fork(consumer1, (void*)4);
	p3->Fork(producer1, (void*)4);
	c3->Fork(consumer1, (void*)3);
	
}


//-----------------------------------------------------------------------------
// Implementation of Producer&Consumer with semaphore.-------------------------
Lock *lock_buff;
Semaphore *sema_full;
Semaphore *sema_empty;
void producer2(int num = 1){
	while(num--){
		sema_empty->P();
		lock_buff->Acquire();
		buffer->Append((void*)1);
		printf("----------%s Produced a new item, now the number of items is %d.\n", currentThread->getName(), buffer->NumInList());
		lock_buff->Release();
		sema_full->V();
	}
	printf("Finished Producing.\n");
}
void consumer2(int num = 1){
	while(num--){
		sema_full->P();
		lock_buff->Acquire();
		buffer->Remove();
		printf("----------%s Consumed an item, now the number of items is %d.\n", currentThread->getName(), buffer->NumInList());
		lock_buff->Release();
		sema_empty->V();
	}
	printf("Finished Consuming.\n");
}


void ThreadTest7(){
	buffer = new List();
	lock_buff = new Lock("sema_buff");
	sema_full = new Semaphore("sema_full", 0);
	sema_empty = new Semaphore("sema_empty", maxnum_buf);
//---------------------------------------------------------
// Easy Test or Complex Test?

/*
	Thread *t1 = new Thread("Thr-Producer");
	Thread *t2 = new Thread("Thr-Consumer");

	t1->Fork(producer2, (void*)8);
	t2->Fork(consumer2, (void*)10);
*/
	Thread *p1 = new Thread("Thr-Producer1");
	Thread *p2 = new Thread("Thr-Producer2");
	Thread *p3 = new Thread("Thr-Producer3");
	Thread *c1 = new Thread("Thr-Consumer1");
	Thread *c2 = new Thread("Thr-Consumer2");
	Thread *c3 = new Thread("Thr-Consumer3");

	p1->Fork(producer2, (void*)4);
	c1->Fork(consumer2, (void*)6);
	p2->Fork(producer2, (void*)5);
	c2->Fork(consumer2, (void*)4);
	p3->Fork(producer2, (void*)4);
	c3->Fork(consumer2, (void*)3);
	
}


//-----------------------------------------------------------------------------
// Implementation of barrior with lock & condition.----------------------------
#define together_num 8
int *working_thread_num;
Lock *mutex_int;
Condition *cond_int;
void runningthr(){
	
	printf("------It's %s(TID: %d) now running! :)\n",currentThread->getName(), currentThread->getTID());
	mutex_int->Acquire();
	(*working_thread_num)++;
	if(*working_thread_num < together_num){
		printf("---There're (%d/%d) threads running to here thus we're blocked.\n", *working_thread_num, together_num);
		cond_int->Wait(mutex_int);
	}
	else if(*working_thread_num == together_num){
		printf("---(%d/%d) threads running to here so we're all waked.\n", *working_thread_num, together_num);
		cond_int->Broadcast(mutex_int);
	}
	
	printf("----------%s(TID: %d) reached the new world and now number of threads is %d.\n", currentThread->getName(), currentThread->getTID(), *working_thread_num);
	
	//printf("Finished Producing.\n");
	mutex_int->Release();
}

void ThreadTest8(){

	working_thread_num = new int(0);
	mutex_int = new Lock("Mutex_Int");
	cond_int = new Condition("Condition_Int");
//---------------------------------------------------------

	Thread* t[8];
	for(int i = 0; i < 8; i++)
		t[i] = new Thread("Thr-test");
	for(int i = 0; i < 8; i++)
		t[i]->Fork(runningthr, 0);

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
	case 6:
	ThreadTest6();
	break;
	case 7:
	ThreadTest7();
	break;
	case 8:
	ThreadTest8();
	break;
 //-----------------------------------------------------------------------------
   default:
	printf("No test specified.\n");
	break;
    }
}

