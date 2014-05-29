// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create several threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustrate the inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.
//

#include <unistd.h>
#include "copyright.h"
#include "system.h"
#include "dinningph.h"
//for lab 5:
#include <time.h>
#include <wait.h>

DinningPh* dp;
//for lab 5:
Semaphore semH("H sem", 1); //initial value = 1
Semaphore semO("O sem", 1); //because they work like mutex
Semaphore semA("A sem", 1);
unsigned int HatomCount = 0;
unsigned int OatomCount = 0;
unsigned int H2OmoleculeCount = 0;

void H(void* name)
{
	printf("%s entrando =]", (char*) name);
	semH.P();
	semO.P();
	semA.P();
	if(HatomCount>0 && OatomCount>0) //it can make water
	{
		--HatomCount;
		--OatomCount;
		printf("\n***Agua! Hice agua! Yo el proceso '%s' hice agua! Yay =D\n", (char*) name);
		++H2OmoleculeCount;
	}
	else //it can not make water, add itself to the list
		++HatomCount;
	//show stats, just to see and check the process
	printf("\t(Hs: %d, Os: %d, As: %d)\n", HatomCount, OatomCount, H2OmoleculeCount);
	semH.V();
	semO.V();
	semA.V();
}

void O(void* name)
{
	printf("%s entrando =)", (char*) name);
	semH.P();
	semO.P();
	semA.P();
	if(HatomCount>1) //it can make water
	{
		HatomCount -= 2;
		printf("\n***Agua! Hice agua! Yo el proceso '%s' hice agua! Yay =D\n", (char*) name);
		++H2OmoleculeCount;
	}
	else //it can not make water, add itself to the list
		++OatomCount;
	//show stats, just to see and check the process
	printf("\t(Hs: %d, Os: %d, As: %d)\n", HatomCount, OatomCount, H2OmoleculeCount);
	semH.V();
	semO.V();
	semA.V();
}

void water()	//for lab 5
{
	// Reinterpret arg "name" as a string
	//char* threadName = (char*)name;
	
	//Create 20 threads, which will randomly be either H atoms or O atoms
	srand(time(NULL));
	for(int num=0; num<20; num++)
	{
		char* threadname = new char[100];
		if(rand()%2)
		{
			sprintf(threadname, "Hilo %d de Hidrógeno", num);
			Thread* hThread = new Thread(threadname);
			hThread->Fork(H, (void*)threadname);
		}
		else
		{
			sprintf(threadname, "Hilo %d de Oxígeno", num);
			Thread* oThread = new Thread(threadname);
			oThread->Fork(O, (void*)threadname);
		}
	}

	//Cool wait function that waits for the next child process to finish
	//use in a cycle to wait for all of them
	int k;
	for(int i=0; i<20; ++i)
	{
		//printf("Esperando al proceso %d\n", i);
        wait(&k);
    }
}

void Philo(void* p)
{

	int eats, thinks;
	long who = (long) p;

	currentThread->Yield();

	for(int i=0; i<10; i++)
	{
		printf(" Philosopher %ld will try to pickup sticks\n", who + 1);

		dp->pickup(who);
		dp->print();
		eats = Random() % 6;

		currentThread->Yield();
		sleep(eats);

		dp->putdown(who);

		thinks = Random() % 6;
		currentThread->Yield();
		sleep(thinks);
	}

}


//----------------------------------------------------------------------
// SimpleThread
// 	Loop 10 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"name" points to a string with a thread name, just for
//	  debugging purposes.
//----------------------------------------------------------------------

void SimpleThread(void* name)
{
	// Reinterpret arg "name" as a string
	char* threadName = (char*)name;
	
	// If the lines dealing with interrupts are commented,
	// the code will behave incorrectly, because
	// printf execution may cause race conditions.
	for(int num=0; num<10; num++)
	{
		//IntStatus oldLevel = interrupt->SetLevel(IntOff);
		printf("*** thread %s looped %d times\n", threadName, num);
		//interrupt->SetLevel(oldLevel);
		//currentThread->Yield();
	}
	//IntStatus oldLevel = interrupt->SetLevel(IntOff);
	printf(">>> Thread %s has finished\n", threadName);
	//interrupt->SetLevel(oldLevel);
}



//----------------------------------------------------------------------
// ThreadTest
// 	Set up a ping-pong between several threads, by launching
//	ten threads which call SimpleThread, and finally calling 
//	SimpleThread ourselves.
//----------------------------------------------------------------------

void ThreadTest()
{
	DEBUG('t', "Entering SimpleTest");

/*
	Thread* Ph;
	dp = new DinningPh();

	for(long k=0; k<5; k++)
	{
		Ph = new Thread("dp");
		Ph->Fork(Philo, (void *) k);
	}

	return;
*/
/*
	for(int k=1; k<=5; k++)
	{
		char* threadname = new char[100];
		sprintf(threadname, "Hilo %d", k);
		Thread* newThread = new Thread(threadname);
		newThread->Fork(SimpleThread, (void*)threadname);
	}
	
	SimpleThread((void*)"Hilo 0");
*/
	//Now, to test the water making
	printf("\n---Now testing the water making---\n");
	water();
}

