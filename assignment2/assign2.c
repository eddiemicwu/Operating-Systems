/*
 * assign2.c
 *
 * Name:Wenbo Wu
 * Student Number:V00928620
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h> 
#include <pthread.h>
#include "train.h"

/*
 * If you uncomment the following line, some debugging
 * output will be produced.
 *
 * Be sure to comment this line out again before you submit 
 */

/* #define DEBUG	1 */
pthread_mutex_t bridgeLock;
pthread_mutex_t lock;
pthread_cond_t  cv;
int turn = 0;	//check for E or W through the bridge.
//LinkedList for arranging the westTrain and eastTrain who arrives first.
struct LinkedList{
	int tid;
	struct LinkedList *next;
};
struct LinkedList *tmp;
struct LinkedList *westTrain;
struct LinkedList *eastTrain;
//count for each train list.
int eCount = 0;
int wCount = 0;

void ArriveBridge (TrainInfo *train);
void CrossBridge (TrainInfo *train);
void LeaveBridge (TrainInfo *train);
/*
 * This function is started for each thread created by the
 * main thread.  Each thread is given a TrainInfo structure
 * that specifies information about the train the individual 
 * thread is supposed to simulate.
 */
void * Train ( void *arguments )
{
	TrainInfo	*train = (TrainInfo *)arguments;

	/* Sleep to simulate different arrival times */
	usleep (train->length*SLEEP_MULTIPLE);

	ArriveBridge (train);
	CrossBridge  (train);
	LeaveBridge  (train); 

	/* I decided that the paramter structure would be malloc'd 
	 * in the main thread, but the individual threads are responsible
	 * for freeing the memory.
	 *
	 * This way I didn't have to keep an array of parameter pointers
	 * in the main thread.
	 */
	free (train);
	return NULL;
}

/*
 * You will need to add code to this function to ensure that
 * the trains cross the bridge in the correct order.
 */
void ArriveBridge ( TrainInfo *train )
{
	printf ("Train %2d arrives going %s\n", train->trainId, 
			(train->direction == DIRECTION_WEST ? "West" : "East"));
	/* Your code here... */
	//section for adding the arrived train to the specific train list.
	pthread_mutex_lock(&lock);
	struct LinkedList *node = (struct LinkedList*)malloc(sizeof(struct LinkedList));
	node->tid = train->trainId;
	node->next = NULL;
	if(train->direction == DIRECTION_WEST)
	{
		if(wCount != 0)
		{
			tmp = westTrain;
			while(tmp->next != NULL) tmp = tmp->next;
			tmp->next = node;
		}
		else
		{			
			westTrain = node;
		}
		wCount++;
	}
	else
	{
		if(eCount != 0)
		{
			tmp = eastTrain;
			while(tmp->next != NULL) tmp = tmp->next;
			tmp->next = node;
		}
		else
		{
			eastTrain = node;
		}
		eCount++;
	}
	pthread_mutex_unlock(&lock);
	//section for checking the turn of the train
	pthread_mutex_lock(&lock);
	if(train->direction == DIRECTION_WEST)
	{
		//check if is this train thread, and check if the eastTrain list is empty or is westTrain turn.
		//Otherwise wait.
		while(!((westTrain->tid == train->trainId) && (eCount == 0 || turn == 2)))
		{
			pthread_cond_wait(&cv, &lock);
		}
	}
	else
	{
		//check if is this train thread, and check if the westTrain list is empty or is eastTrain turn.
		//Otherwise wait.
		while(!((eastTrain->tid == train->trainId) && (wCount == 0 || turn < 2)))
		{
			pthread_cond_wait(&cv, &lock);
		}
	}
	pthread_mutex_unlock(&lock);
	pthread_mutex_lock(&bridgeLock);//Lock the bridge.
}

/*
 * Simulate crossing the bridge.  You shouldn't have to change this
 * function.
 */
void CrossBridge ( TrainInfo *train )
{
	printf ("Train %2d is ON the bridge (%s)\n", train->trainId,
			(train->direction == DIRECTION_WEST ? "West" : "East"));
	fflush(stdout);
	
	/* 
	 * This sleep statement simulates the time it takes to 
	 * cross the bridge.  Longer trains take more time.
	 */
	usleep (train->length*SLEEP_MULTIPLE);

	printf ("Train %2d is OFF the bridge(%s)\n", train->trainId, 
			(train->direction == DIRECTION_WEST ? "West" : "East"));
	fflush(stdout);
}

/*
 * Add code here to make the bridge available to waiting
 * trains...
 */
void LeaveBridge ( TrainInfo *train )
{
	pthread_mutex_lock(&lock);
	if(train->direction == DIRECTION_WEST)
	{
		tmp = westTrain;
		westTrain = westTrain->next;
		if(turn == 2)
		{
			turn = 0;
		}
		wCount--;
	}
	else
	{
		tmp = eastTrain;
		eastTrain = eastTrain->next;
		if(wCount != 0 && turn < 2)
		{
			turn = turn + 1;
		}
		eCount--;
	}
	free(tmp);
	pthread_cond_broadcast(&cv);
	pthread_mutex_unlock(&lock);
	pthread_mutex_unlock(&bridgeLock);//Unlock the bridge
}

int main ( int argc, char *argv[] )
{
	int		trainCount = 0;
	char 		*filename = NULL;
	pthread_t	*tids;
	int		i;

	/* Parse the arguments */
	if ( argc < 2 )
	{
		printf ("Usage: part1 n {filename}\n\t\tn is number of trains\n");
		printf ("\t\tfilename is input file to use (optional)\n");
		exit(0);
	}
	
	if ( argc >= 2 )
	{
		trainCount = atoi(argv[1]);
	}
	if ( argc == 3 )
	{
		filename = argv[2];
	}	
	//initialize mutex and condition variable.
	if(pthread_mutex_init(&lock, NULL)!= 0)
	{
		printf("Mutex init failed.\n");
		return 1;
	}
	if(pthread_mutex_init(&bridgeLock, NULL)!= 0)
	{
		printf("Bridge_Mutex init failed.\n");
		return 1;
	}
	if(pthread_cond_init(&cv, NULL)!= 0)
	{
		printf("Condition init failed.\n");
		return 1;
	}
	initTrain(filename);
	
	/*
	 * Since the number of trains to simulate is specified on the command
	 * line, we need to malloc space to store the thread ids of each train
	 * thread.
	 */
	tids = (pthread_t *) malloc(sizeof(pthread_t)*trainCount);

	/*
	 * Create all the train threads pass them the information about
	 * length and direction as a TrainInfo structure
	 */
	for (i=0;i<trainCount;i++)
	{
		TrainInfo *info = createTrain();
		
		printf ("Train %2d headed %s length is %d\n", info->trainId,
			(info->direction == DIRECTION_WEST ? "West" : "East"),
			info->length );

		if ( pthread_create (&tids[i],0, Train, (void *)info) != 0 )
		{
			printf ("Failed creation of Train.\n");
			exit(0);
		}
	}

	/*
	 * This code waits for all train threads to terminate
	 */
	for (i=0;i<trainCount;i++)
	{
		pthread_join (tids[i], NULL);
	}
	
	pthread_mutex_destroy(&bridgeLock);
	pthread_mutex_destroy(&lock);
	pthread_cond_destroy(&cv);
	free(tids);
	return 0;
}

