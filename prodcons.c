/* 
 * Operating Systems   (2INCO)   Practical Assignment
 * Condition Variables Application
 *
 * STUDENT_NAME_1 (STUDENT_NR_1)
 * STUDENT_NAME_2 (STUDENT_NR_2)
 *
 * Grading:
 * Students who hand in clean code that fully satisfies the minimum requirements will get an 8. 
 * "Extra" steps can lead to higher marks because we want students to take the initiative. 
 * Extra steps can be, for example, in the form of measurements added to your code, a formal 
 * analysis of deadlock freeness etc.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#include "prodcons.h"

/* buffer[]
 * circular buffer that holds the items generated by the producer and
 * which have to be retrieved by consumers
 */
static ITEM   buffer [BUFFER_SIZE];
pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t prodCondition = PTHREAD_COND_INITIALIZER;
static pthread_cond_t conCondition/*[NROF_CONSUMERS]*/ = PTHREAD_COND_INITIALIZER;
int bufferCounter = 0;

static void rsleep (int t);



/* producer thread */
static void * producer (void * arg)
{
    ITEM    item;   // a produced item
    int itemsProduced = 1;
    
    while (itemsProduced < NROF_ITEMS)
    {
        rsleep (PRODUCER_SLEEP_FACTOR);

       	if(bufferCounter >= BUFFER_SIZE) {
            pthread_mutex_lock(&bufferMutex);
            printf("Producer waiting for condition\n");
           	pthread_cond_wait(&prodCondition, &bufferMutex);
            printf("Producer started to fill buffer\n");
            //fill buffer
       		int i;
       		for(i = 0; i < BUFFER_SIZE; i++){
            	int randomDestination = (rand() % NROF_CONSUMERS) + 1;
            	int itemShift = itemsProduced << NROF_BITS_DEST;
            	item = itemShift | randomDestination;
            	itemsProduced++;
            	buffer[i] = item;
            	printf("%04x\n", item); // write info to stdout
       		}
       		bufferCounter = 0;
            //end buffer
           	//TODO: which consumer?
            //printf("thread: leave CS\n");
            pthread_mutex_unlock(&bufferMutex);
       		/*for(i = 0; i < BUFFER_SIZE; i++){
       			pthread_cond_signal(&conCondition); //Before or after the lock? Thats the question
       		}*/
       	} else {
       		pthread_cond_signal(&conCondition);
       	}
        // TODO: 
        // * produce new item and put it into buffer[]
        //
        // use shifting and masking to put a 'seq' and a 'dest' in an item
        // (see mask_test() in condition_basics.c how to create bit masks)
        //
        // follow this pseudocode (according to the ConditionSynchronization lecture):
        //      mutex-lock;
        //      while not condition-for-this-producer
        //          wait-cv;
        //      critical-section;
        //      possible-cv-signals;
        //      mutex-unlock;
        //
        // (see condition_test() in condition_basics.c how to use condition variables)

        // apply this printf at the correct location in that pseudocode:
    }
    printf("Producer done\n");
    // TODO: 
    // * inform consumers that we're ready
    return(NULL);
}

/* consumer thread */
static void * consumer (void * arg)
{
    ITEM    item;   // a consumed item
    int     id = *((int*)arg);     // identifier of this consumer (value 0..NROF_CONSUMERS-1)
    
    while (1)
    {
        rsleep (100 * NROF_CONSUMERS);
        if(bufferCounter >= BUFFER_SIZE){
        	pthread_cond_signal(&prodCondition);
        } else {
        	pthread_mutex_lock(&bufferMutex);
        	printf("Consumer achieved successful lock and is waiting for the condition\n");
        	pthread_cond_wait(&conCondition, &bufferMutex); //What is the signal it needs to wait for?
        	printf("Consumer started to empty buffer\n");
        	item = buffer[bufferCounter];
        	buffer[bufferCounter] = 0;
        	bufferCounter++;
        	pthread_mutex_unlock(&bufferMutex);
        }


        // TODO: get the next item from buffer[] (intended for this customer)
        //
        // follow this pseudocode (according to the ConditionSynchronization lecture):
        //      mutex-lock;
        //      while not condition-for-this-consumer
        //          wait-cv;
        //      critical-section;
        //      possible-cv-signals;
        //      mutex-unlock;

        // apply this printf at the correct location in that pseudocode:
        printf("%*s    C%d:%04x\n", 7*id, "", id, item); // write info to stdout (with indentation)
    }
    return(NULL);
}

int main (void)
{
    // TODO: 
    // * startup the producer thread and the consumer threads
    // * wait until all threads are finished  
    // (see assignment Threaded Application how to create threads and how to wait for them)
	pthread_t producer_id, consumer_id;
	int con_id = 1;
	int newThread = pthread_create(&producer_id, NULL, producer, NULL);
	if(newThread == -1){
		perror("Creating the producer thread failed");
		exit(1);
	}
	printf("Producer created\n");
	newThread = pthread_create(&consumer_id, NULL, consumer, &con_id);
	if(newThread == -1){
		perror("Creating a consumer thread failed");
		exit(1);
	}
	printf("Consumer created\n");
	sleep(1);
	pthread_cond_signal(&prodCondition);
	pthread_join(producer_id, NULL);
	pthread_join(consumer_id, NULL);
    return (0);
}

/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
static void rsleep (int t)
{
    static bool first_call = true;
    
    if (first_call == true)
    {
        srandom (time (NULL) % getpid ());
        first_call = false;
    }
    usleep (random() % t);
}

