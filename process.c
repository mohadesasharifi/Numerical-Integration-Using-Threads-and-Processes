#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>     /* defines fork() */
#include <sys/wait.h>
#include <time.h> 

#define MAX_CHILDREN 6
static int numChildren = 0;

typedef double MathFunc_t(double);


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
double integrateTrap(MathFunc_t* func, double rangeStart, double rangeEnd, size_t numSteps)
{
	double rangeSize = rangeEnd - rangeStart;
	double dx = rangeSize / numSteps;

	double area = 0;
	for (size_t i = 0; i < numSteps; i++) {
		double smallx = rangeStart + i*dx;
		double bigx = rangeStart + (i+1)*dx;

		area += dx * ( func(smallx) + func(bigx) ) / 2; //Would be more efficient to multiply area by dx once at the end. 
	}

	return area;
}


bool getValidInput(double* start, double* end, size_t* numSteps, size_t* funcId)
{
	printf("Query: [start] [end] [numSteps] [funcId]\n");

	//Read input numbers and place them in the given addresses:
	size_t numRead = scanf("%lf %lf %zu %zu", start, end, numSteps, funcId);

	//Return whether the given range is valid:
	return (numRead == 4 && *end >= *start && *numSteps > 0 && *funcId < NUM_FUNCS);
}


/**
 * Signal handler for a child process ending
 */
void waitChild() {
    if (wait(NULL)) {
		numChildren--;
	} 
    
    
    
}


int main(void)
{
	time_t start, end;
	time(&start);
    double rangeStart;
    double rangeEnd;
    size_t numSteps;
    size_t funcId;

    // Set the handler for when a child process ends
    signal(SIGCHLD, waitChild);

    while(1) {
        // Only ask for input if another child is allowed
        if (numChildren < MAX_CHILDREN) {
            // Ask for new input, if valid then create a new child process to integrate with
            if (getValidInput(&rangeStart, &rangeEnd, &numSteps, &funcId)) {
                int childPid = fork(); // Create a new child process
				// If child code then integrate the trapezoid and print the result
				if (childPid == 0) {
					double area = integrateTrap(FUNCS[funcId], rangeStart, rangeEnd, numSteps);

					printf("The integral of function %zu in range %g to %g is %.10g\n", funcId, rangeStart, rangeEnd, area);
					exit(0);

				// If an error creating child then exit the program
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

    // Wait for all child processes to finish
    while (wait(NULL) > 0) {
        continue;
    }
	time(&end); 
 
    // Calculating total time taken by the program. 
    double time_taken = (double)(end - start); 
    printf("Time taken by program is : %g sec\n", time_taken); 

	exit(0);
}