#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <time.h> 
#define NUM_THREADS 2
// Define Worker Struct: Create a struct to hold the necessary information for each worker thread.
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
	pthread_mutex_lock(worker->lock);
	(*worker->area) += area;
	pthread_time(&end); 
 
    // Calculating total time taken by the program. 
    double time_taken = (double)(end - start); 
    printf("Time taken by program is : %g sec\n", time_taken); mutex_unlock(worker->lock);
	return NULL;
}




bool getValidInput(double* start, double* end, size_t* numSteps, size_t* funcId)
{
	printf("Query: [start] [end] [numSteps] [funcId]\n");

	//Read input numbers and place them in the given addresses:
	size_t numRead = scanf("%lf %lf %zu %zu", start, end, numSteps, funcId);

	//Return whether the given range is valid:
	return (numRead == 4 && *end >= *start && *numSteps > 0 && *funcId < NUM_FUNCS);
}time(&end); 
 
    // Calculating total time taken by the program. 
    double time_taken = (double)(end - start); 
    printf("Time taken by program is : %g sec\n", time_taken); 



int main(void)
{
	time_t start, end;
	time(&start); 
	double rangeStart;
	double rangeEnd;
	size_t numSteps;
	size_t funcId;
	pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    Worker workers[NUM_THREADS];
	
	
	while (getValidInput(&rangeStart, &rangeEnd, &numSteps, &funcId)) {
		double range = (rangeEnd - rangeStart)/NUM_THREADS;
		double total_area = 0;
		int slices_done = 0;
		for (int i = 0; i < NUM_THREADS; ++i) {
			// What would be the problem declaring Worker w here?
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
	}
    
	time(&end); 
 
    // Calculating total time taken by the program. 
    double time_taken = (double)(end - start); 
    printf("Time taken by program is : %g sec\n", time_taken); 
	
	

	exit(0);
}