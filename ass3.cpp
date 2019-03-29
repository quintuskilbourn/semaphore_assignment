/*
CS3103 Assignment 2
Completed by:
Quintus Kilbourn (54871935)
Rohit Shyla Kumar (54581876)
File Name - 54871935_54581876.cpp
Description - Multithreaded implementation of a FIFO queue
*/

#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <semaphore.h>
#include "MyQueue.h"

#define SSLEEP 2000000


using namespace std;

//Global variables
static pthread_mutex_t qmtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t smtx = PTHREAD_MUTEX_INITIALIZER;
sem_t empty;
int servTok = 0;
int queueLen = 0;
int totFetch = 0;
int currentSeq = 0; ///////added new var//////////
int pflowed = 0;

//Function to serve a single token
bool servIncrement(MyQueue* queue, int &fetched, int &maxC,int &totFetch)		////DONT HAVE TO PASS TOTFETCH IF GLOBAL VAR
{
	//Ensure the queue is not empty
	if(queue->DeQueue()==-1)
	{
		cout<<"DeQueue not successfull"<<endl;
		return 0;
	}
	fetched++;
	totFetch++;
	queueLen--;
	servTok++;
	//Case - served as many tokens as required by the user input
	//Action - return successful
	if(servTok>=maxC)
	{
		return 1;
	}
	
	return 0;
}

//Function to parse thread arguments
//Commong to server and flow threads that use the same set of arguments
void parseArgs(void *args,MyQueue* &queue,int &maxC,int &flowInt)
{
	//Typecast the void pointers to pointer types we require and extract accordingly
	MyQueue** toQ = (MyQueue**)args;
	queue = toQ[1];
	int** toInt = (int**)args;
	maxC = *(toInt[2]);
	flowInt = *(toInt[0]);
}







//Thread function to serve tokens removed from the queue
void *serve(void *args)
{
	//Parse arguments
	MyQueue* queue;
	int maxC, flowInt;
	parseArgs(args, queue, maxC, flowInt);
	static int fetched;
	//int totFetch = 0;
	//Run until as many tokens as the user desires are served
	while(servTok<maxC)
	{
		int ran = rand()%20+1;				//generate random number between 1 and 20 to determine number of tokens to serve
		fetched = 0;

		//Lock the mutex so elements will not be inserted into the queue by other threads while being served
		if(pthread_mutex_lock(&qmtx))
		{
			cout<<"Mutex Lock Error"<<endl;
			exit(-1);
		}
		

		while(1){
			if(!(queue->isEmpty()))
			{
				if(servIncrement(queue, fetched, maxC,totFetch))
			 		{
			 			break;
					}
			}
			else
			{
				sem_post(&empty);
				//cout<<"_________Sem Posted________"<<endl;
				break;
			}
		}


// 		if(ran >= queueLen)
// 		{
// 			while(!(queue->isEmpty()))
// 			{
// 				if(servIncrement(queue, fetched, maxC,totFetch))
// 				{
// 					break;
// 				}
// 			}
// 			sem_post(&empty);
// 		//	int outval;
// 		//	sem_getvalue(&empty, &outval);

// 		//	cout<<"________SEM POSTED____"<<outval<<endl;
// 		}
// 		//Case - there are enough tokens present to be served
// 		//Action - serve as many tokens as required
// 		else
// 		{
// 			for(int i = 0; i<ran; i++)
// 			{
// 				if(servIncrement(queue, fetched, maxC,totFetch))
// 				{
// 					break;
// 				}
// 			}
// 		}



		cout<<"\t\t\t\t\t"<<queueLen<<"\t\t"<<fetched<<"\t\t"<<totFetch<<endl;

		//Unlock the mutex so other threads may access the queue again
		if(pthread_mutex_unlock(&qmtx))
		{
			cout<<"Mutex Unlock Error"<<endl;
			exit(-1);
		}

		//Sleep for a constant 2 second time
		usleep(SSLEEP);
	}
	sem_post(&empty); ////not final solution
	//void *retVal = &totFetch;
	return (void*)1;
}








//Thread function to generate tokens and insert them into the queue
void *flow(void *args)
{
	//Parse arguments
	MyQueue* queue;
	int maxC, flowInt;
	parseArgs(args,queue,maxC,flowInt);
	//static int count = 0;				//static count to track sequence number of tokens being inserted into the queue

	//Run until as many tokens as the user desires are served
	while(servTok<maxC)
	{
		int ran = rand()%10+1;			//generate random number between 1 and 10 to determine number of tokens to be inserted
		int placed = 0;					//track number of tokens actually placed in the queue incase of overflow

		//Lock the mutex so elements will not be removed from the queue by other threads while being inserted
		if(pthread_mutex_lock(&qmtx))
		{
			cout<<"Mutex Lock Error"<<endl;
			exit(-1);
		}

		/////////////////////////////////
		if(pthread_mutex_lock(&smtx))
		{
			cout<<"Mutex Lock Error"<<endl;
			exit(-1);
		}
		/////////////////////////////////


		for(int i = 0; i<ran;i++)
		{
			//case - queue is not full
			//action - insert tokens into the queue
			if(!(queue->isFull()))
			{
				queue->EnQueue(currentSeq);
				currentSeq++;
				queueLen++;
				placed++;
			}
			//case - queue is full
			//action - serve overflowed tokens
			else
			{
				servTok++;
				if(servTok>=maxC)
				{
					break;
				}
			}
		}
		cout<<placed<<"(FLOW)\t\t"<<currentSeq-1<<"\t\t\t"<<queueLen<<endl;

		//////////////////////////////////////////////
		if(pthread_mutex_unlock(&smtx))
		{
			cout<<"Mutex Unlock Error"<<endl;
			exit(-1);
		}
		/////////////////////////////////////////

		//Unlock the mutex so other threads may access the queue again
		if(pthread_mutex_unlock(&qmtx))
		{
			cout<<"Mutex Unlock Error"<<endl;
			exit(-1);
		}

		//Sleep for a user defined time period
		usleep(flowInt*1000000);
	}
	return (void *) 1;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

void*pflow(void *args){
	MyQueue* queue;
	int maxC, flowInt;
	parseArgs(args, queue, maxC, flowInt);

	while(servTok<maxC)
	{
//		cout<<"_________Sem Wait Called________"<<endl;

		sem_wait(&empty);
		
		if(servTok>=maxC)
			return (void*) -1;
	//	int outval;
	//	if(sem_getvalue(&empty, &outval)==-1){
	//		cout<<"makaka"<<endl;
	//	}
	//	cout<<"_________Sem Wait Passed________"<<outval<<endl;

		///////////////////////////////////
		//ALL OF THE FOLLOWING CODE IS THE SAME AS IN FLOW EXCEPT THE RAN RANGE
		///////////////////////////////////

		int ran = rand()%5+1;			//generate random number between 1 and 10 to determine number of tokens to be inserted
		int placed = 0;					//track number of tokens actually placed in the queue incase of overflow

		//Lock the mutex so elements will not be removed from the queue by other threads while being inserted
		if(pthread_mutex_lock(&qmtx))
		{
			cout<<"Mutex Lock Error"<<endl;
			exit(-1);
		}

		/////////////////////////////////
		if(pthread_mutex_lock(&smtx))
		{
			cout<<"Mutex Lock Error"<<endl;
			exit(-1);
		}
		/////////////////////////////////

		for(int i = 0; i<ran;i++)
		{
			//case - queue is not full
			//action - insert tokens into the queue
			if(!(queue->isFull()))
			{
				queue->EnQueue(currentSeq);
				currentSeq++;
				queueLen++;
				placed++;
			}
			//case - queue is full
			//action - serve overflowed tokens
			else
			{
				servTok++;
				if(servTok>=maxC)
				{
					break;
				}
			}
		}
		cout<<placed<<"(PFLOW)\t"<<currentSeq-1<<"\t\t\t"<<queueLen<<endl;
		pflowed+=placed;
/////////////////////////////////////////
		if(pthread_mutex_unlock(&smtx))
		{
			cout<<"Mutex Unlock Error"<<endl;
			exit(-1);
		}
////////////////////////////////////////////

		//Unlock the mutex so other threads may access the queue again
		if(pthread_mutex_unlock(&qmtx))
		{
			cout<<"Mutex Unlock Error"<<endl;
			exit(-1);
		}
	}


	return (void*)1;
}







/*
Main function
Main thread creates other threads and displays outputs
*/
int main(int argc, char* argv[])
{
	//Extracting command line arguments for max count of tokens to be served and flow interval
	//abs() ensures no negative values crash the program
	int maxC = abs(strtol(argv[1],NULL,10));
	int flowInt = abs(strtol(argv[2],NULL,10));

	//INITIALISE SEMAPHORES

	sem_init(&empty, 0, 0);

	//Store thread ids of different threads
	pthread_t flowId, servId, pflowId;
	MyQueue* queue = new MyQueue();

	//Prepare arguments as a void pointer to pass to thread functions
	void* args[] = {&flowInt, queue, &maxC};

	//Print header with appropriate spacing using horizontal tabs
	cout<<"The Max Token is "<< maxC << " and the interval time for the flow is "<< flowInt<<endl;
	cout<<"Flow\t\t\t\t\tMyQueue\t\tServer"<<endl;
	cout<<"Token Added\tLatest Sequence Num\tCurrent Length\tToken Fetched\tTotal Token Fetched"<<endl;


	//set the seed of the random number generator to the current time to produce more random results
	srand(time(NULL));

	//Create threads and handle errors
	if(pthread_create(&flowId,NULL,flow,args))
	{
		cout<<"Failed to create flow thread"<<endl;
		exit(-1);
	}

	if(pthread_create(&servId,NULL,serve,args))
	{
		cout<<"Failed to create server thread"<<endl;
		exit(-1);
	}

	if(pthread_create(&pflowId,NULL,pflow,args))
	{
		cout<<"Failed to create server thread"<<endl;
		exit(-1);
	}


	//void* retVal;						//temporarily store return value from server thread

	//Wait for the thread functions to return before continuing with the main thread's functions
	pthread_join(servId,NULL);
	pthread_join(flowId,NULL);
	pthread_join(pflowId,NULL);
	sem_destroy(&empty);
	//int totFetch = *((int *)retVal);	//convert value returned from server thread
	////////////////////////////////////////////////
	//TODO get rid of global vars!!!!!!!!!!!!///////
	///////////////////////////////////////////////
	//Print final results
	cout << "The total number of token that have been fetched by the server is " << totFetch << endl;
	cout<<"The total number of token that have been generated by the flow is " << servTok + queueLen - pflowed << endl;
	cout<<"The total number of token that have been generated by the pflow is " <<	pflowed <<endl;
	cout << "The total number of token that have been dropped by the queue is " << maxC- totFetch << endl;

	return 0;
}
