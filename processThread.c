#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>     /* defines fork() */
#include <sys/wait.h>
#include <signal.h>

#define NUM_THREADS 4
#define MAX_CHILDREN 4
static int numChildren = 0;

// Define Worker Struct: Create a struct to hold the necessary information for each worker thread.
//why func and area are pointers
typedef double MathFunc_t(double);

typedef struct {
    MathFunc_t* func;
    double rangeStart;
	double rangeEnd;
	size_t numSteps;
	double *area;
    pthread_mutex_t *lock;
    pthread_t thread;

} Worker;



double gaussian(double x)
{
	return exp(-(x*x)/2) / (sqrt(2 * M_PI));
}


double chargeDecay(double x)
{
	if (x < 0) {
		return 0;
	} else if (x < 1) {
		return 1 - exp(-5*x);
	} else {
		return exp(-(x-1));
	}
}

#define NUM_FUNCS 3
static MathFunc_t* const FUNCS[NUM_FUNCS] = {&sin, &gaussian, &chargeDecay};




//why integrateTrape is *integrateTrap?
//Integrate using the trapezoid method. 
void *integrateTrap(void *ptr)
{
	Worker *worker = (Worker*)ptr;
	double rangeEnd = worker->rangeEnd;
	double rangeStart = worker->rangeStart;
	size_t numSteps = worker->numSteps;
	MathFunc_t *func = worker->func;
	double rangeSize = rangeEnd - rangeStart;
	double dx = rangeSize / numSteps;

	double area = 0;
	for (size_t i = 0; i < numSteps; i++) {
		double smallx = rangeStart + i*dx;
		double bigx = rangeStart + (i+1)*dx;

		area += dx * ( func(smallx) + func(bigx) ) / 2; //Would be more efficient to multiply area by dx once at the end. 
	}
	//why pthread_mutex_lock gets the worker->lock as the parameter?
	pthread_mutex_lock(worker->lock);
	//why this syntax? (*)
	(*worker->area) += area;
	pthread_mutex_unlock(worker->lock);
	return NULL;
}




bool getValidInput(double* start, double* end, size_t* numSteps, size_t* funcId)
{
	printf("Query: [start] [end] [numSteps] [funcId]\n");

	//Read input numbers and place them in the given addresses:
	size_t numRead = scanf("%lf %lf %zu %zu", start, end, numSteps, funcId);

	//Return whether the given range is valid:
	return (numRead == 4 && *end >= *start && *numSteps > 0 && *funcId < NUM_FUNCS);
}

void runMultiThreadInChild(double rangeStart, double rangeEnd, size_t numSteps, size_t funcId) {
	pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	Worker workers[NUM_THREADS];
	double range = (rangeEnd - rangeStart)/NUM_THREADS;
	double total_area = 0;
	int slices_done = 0;
	for (int i = 0; i < NUM_THREADS; ++i) {
		Worker *worker = &workers[i];
		worker->area = &total_area; // Pass the global total into each thread
		worker->func = FUNCS[funcId];
		worker->lock = &lock;
		
		if (i == NUM_THREADS -1) {
			worker->numSteps = numSteps - slices_done;
		} else {
			worker->numSteps = ceil(numSteps / NUM_THREADS);
			slices_done += worker->numSteps;
		}
		//Make integrateTrap run in a thread!
		worker->rangeStart = rangeStart + range  * i;
		worker->rangeEnd = rangeStart + range  * (i+1);
		pthread_create(&worker->thread, NULL, integrateTrap, (void*)worker);
	}

	////////////////////////////////
	// Wait for all the threads we created
	for (int i = 0; i < NUM_THREADS; ++i) {
		// Wait for ith thread to finish
		pthread_join(workers[i].thread, NULL);
	}
	///////
	printf("The integral of function %zu in range %g to %g is %.10g\n", funcId, rangeStart, rangeEnd, total_area);
	exit(0);
}
/**
 * Signal handler for a child process ending
 */
void waitChild() {
    if (wait(NULL) > 0)
    {
        // Child process finished
        numChildren--;
    }
}

int main(void)
{
	double rangeStart;
	double rangeEnd;
	size_t numSteps;
	size_t funcId;
	
	signal(SIGCHLD, waitChild);
	while(1) {
		/*if max number of child process has not reached, parent process can take new input and create
		 another child process to calculate */ 
		if (numChildren < MAX_CHILDREN) {
			if (getValidInput(&rangeStart, &rangeEnd, &numSteps, &funcId)) {
				int childPid = fork(); // Create a new child process
				// If child code then run integrateTrap in multithreaded program and print the result
				if (childPid == 0) {
					runMultiThreadInChild(rangeStart, rangeEnd, numSteps, funcId);
				// If error happened 
				} else if (childPid < 0) {
					printf("Error creating child process");
					exit(0);

				/*If parent code, asking for user input, increment the number of active children by one 
				because a new child process will take the job of calculating the integration.*/ 
				} else {
					numChildren++;
				}
			// If invalid input then break the while loop	
			} else {
				break;
			}
		}
	}
	
	// Wait for all child processes to finish before the parent process 
	while (wait(NULL) > 0) {
        continue;
    }
	exit(0);
}