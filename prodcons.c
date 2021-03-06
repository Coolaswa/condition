/* 
 * Operating Systems   (2INCO)   Practical Assignment
 * Condition Variables Application
 *
 * Michiel Favier (0951737)
 * Diederik de Wit (0829667)
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#include "prodcons.h"

static ITEM   buffer [BUFFER_SIZE];
pthread_mutex_t prodMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t prodCondition = PTHREAD_COND_INITIALIZER;
static pthread_cond_t conCondition[NROF_CONSUMERS];
pthread_cond_t placeHolder = PTHREAD_COND_INITIALIZER;
pthread_mutex_t conMutex[NROF_CONSUMERS];

static void rsleep (int t);

/* producer thread */
static void * producer (void * arg)
{
    ITEM    item;   //A produced item.
    int itemsProduced = 1; //The item which is going to be produced next. 
    bool bufferEmpty = true; //A boolean which signifies whether the buffer is empty or not.
	
	/*
	* While loop which keeps the buffer filled and assigns/signals the consumer threads.
	* The produced thread will exit this loop when all items have been produced and the buffer is empty.
	* When all items have been produced and the buffer is empty, the produced thread can be exited.
	*/

    pthread_mutex_lock(&prodMutex);
    while (itemsProduced < NROF_ITEMS || !bufferEmpty)
    {
        rsleep (PRODUCER_SLEEP_FACTOR);
	    
	    //Check whether the buffer is empty or not.
        if(buffer[BUFFER_SIZE - 1] == 0){
        	bufferEmpty = true;
        } else {
        	bufferEmpty = false;
        }
	    
	    //If the buffer is empty and not all items have been produced yet, fill the buffer.
       	if(bufferEmpty && itemsProduced < NROF_ITEMS) {
		
            printf("Producer started to fill buffer\n");
		
            //Produce an item and copy it into the buffer, for the entire buffer
       		int i;
       		for(i = 0; i < BUFFER_SIZE; i++){
			//Generate a random number between 1 and NROF_CONSUMERS, to determine the destination of the item.
            	unsigned short int randomDestination = (rand() % NROF_CONSUMERS) + 1;
			//Combine this with a sequential number into item
            	unsigned short int itemShift = (unsigned short int)itemsProduced << NROF_BITS_DEST;
            	item = itemShift | randomDestination;
			//Increment the counter which keeps track of which items have been produced.
            	itemsProduced++;
			//Put the produced item into the buffer
            	buffer[i] = item;
            	printf("%04x\n", item); // write info to stdout
       		}
       		bufferEmpty = false;
       	} else /*The buffer is not empty, so assign the next item to a consumer*/{
       		int i;
		//Go through the buffer
			for(i = 0; i < BUFFER_SIZE; i++){
				/*
				* The first place in the buffer with an item will be the oldest.
				* Assign this item to the correct consumer.
				*/
				if (buffer[i] != 0) {
					printf("Producer waiting for thread to signal\n");
					pthread_cond_wait(&prodCondition, &prodMutex);
					//Extract the destination from the item.
					ITEM locItem = buffer[i];
					unsigned short int dest = locItem & ((unsigned short int)~0 >> (16-NROF_BITS_DEST));
					//Signal the correct consumer, but wait for a signal.
					pthread_mutex_lock(&conMutex[dest]);
					pthread_cond_signal(&conCondition[dest]);
					pthread_mutex_unlock(&conMutex[dest]);
					printf("Just signaled consumer %d\n", dest);
					break;
				}
			}
       	}
    }
    printf("Producer done\n");
    pthread_mutex_unlock(&prodMutex);
    // TODO: Exit produced thread
    return(NULL);
}

/* consumer thread */
static void * consumer (void * arg)
{
	int itemsConsumed = 1; //The item which will be consumed next.
    ITEM    item;   // a consumed item
    int     id = *((int*)arg);     // identifier of this consumer (value 0..NROF_CONSUMERS-1)
    printf("My id is: %d\n", id);
    pthread_mutex_lock(&conMutex[id]);
    
	//While not all items have been consumed, continue going through the loop.
    while (itemsConsumed < NROF_ITEMS) //TODO: This does not end the consumer, ever.
    {
        rsleep (100 * NROF_CONSUMERS);
        
	    /*
	    * Go through the buffer, from oldest to newest.
	    * If buffer[i] = 0, it means that there is no item in this place.
	    * The consumer will continue going through the buffer until it finds the oldest item present.
	    */

    	printf("Consumer %d achieved successful lock and is waiting for the condition\n", id);
    	pthread_cond_wait(&conCondition[id], &conMutex[id]);
        int i;
	    for (i = 0; i < BUFFER_SIZE; i++)
	    {
		    //if an item is found
		   	if (buffer[i] != 0) //We could include some check to make sure the consumer takes the right item.
		   	{

		    	printf("Consumer %d taking one item from the buffer\n", id);
				//Consume the item
		    	item = buffer[i];
				//Delete the item
		    	buffer[i] = 0;
				//Increment the counter which keeps track of how many items have been consumed.
		    	itemsConsumed++;
		    	printf("%*s    C%d:%04x\n", 7*id, "", id, item); // write info to stdout (with indentation)
				//Check the next item to see if it also belongs to this consumer
		    	if(((buffer[i + 1] & ((unsigned short int)~0 >> (16-NROF_BITS_DEST))) == id) && i + 1 < BUFFER_SIZE){
		    		printf("The next item also belongs to me!\n");
		    		continue;
		    	} else {
		    		break;
		    	}
		   	}
		    //If the end of the buffer has been reached and no items were found
	    }
	    pthread_mutex_lock(&prodMutex);
	    pthread_cond_signal(&prodCondition);
	    pthread_mutex_unlock(&prodMutex);
	    printf("Signaling producer we've finished\n");
    }
    printf("Consumer %d done\n", id);
    pthread_mutex_unlock(&conMutex[id]);
	//TODO: Exit consumer thread
    return(NULL);
}

int main (void)
{
	//Initialize the consumer conditions
	int i;
	for(i = 0; i < NROF_CONSUMERS; i++){
		conCondition[i] = placeHolder;
		conMutex[i] = prodMutex;
	}
	pthread_t producer_id, consumer_id[NROF_CONSUMERS];
	//Create the produced thread.
	int newThread = pthread_create(&producer_id, NULL, producer, NULL);
	//Show an error and exit, if creating the producer thread failed.
	if(newThread == -1){
		perror("Creating the producer thread failed");
		exit(1);
	}
	printf("Producer created\n");
	//Create the consumer threads, according to NROF_CONSUMERS.
	int con_id[NROF_CONSUMERS];
	//For every consumer
	for(i = 1; i <= NROF_CONSUMERS; i++ ){
		//Assign the consumer id
		con_id[i] = i;
		//Create the consumer thread
		newThread = pthread_create(&consumer_id[i], NULL, consumer, &con_id[i]);
		//Show an error and exit, if creating the consumer thread failed.
		if(newThread == -1){
			perror("Creating a consumer thread failed");
			exit(1);
		}
		printf("Consumer %d created\n", i);
	}
	sleep(1);
	
	//Signal the producer, to kickstart production
	pthread_cond_signal(&prodCondition);
	pthread_join(producer_id, NULL);
	for(i = 1; i <= NROF_CONSUMERS; i++){
		pthread_join(consumer_id[i], NULL);
	}
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

