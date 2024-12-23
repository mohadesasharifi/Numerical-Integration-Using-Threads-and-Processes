#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

typedef double MathFunc_t(double);

double gaussian(double x)
{
    return exp(-(x * x) / 2) / (sqrt(2 * M_PI));
}

double chargeDecay(double x)
{
    if (x < 0)
    {
        return 0;
    }
    else if (x < 1)
    {
        return 1 - exp(-5 * x);
    }
    else
    {
        return exp(-(x - 1));
    }
}

#define NUM_FUNCS 3
static MathFunc_t *const FUNCS[NUM_FUNCS] = {&sin, &gaussian, &chargeDecay};

#define MAX_CHILDREN 2

static volatile int numChildren = 0;

void waitChild(int sigNum)
{
    // (void)sigNum; // Suppress unused parameter warning
    while (wait(NULL) > 0)
    {
        // Child process finished
        numChildren--;
    }
}

double integrateTrap(MathFunc_t *func, double rangeStart, double rangeEnd, size_t numSteps)
{
    double rangeSize = rangeEnd - rangeStart;
    double dx = rangeSize / numSteps;

    double area = 0;
    for (size_t i = 0; i < numSteps; i++)
    {
        double smallx = rangeStart + i * dx;
        double bigx = rangeStart + (i + 1) * dx;

        area += dx * (func(smallx) + func(bigx)) / 2;
    }

    return area;
}

bool getValidInput(double *start, double *end, size_t *numSteps, size_t *funcId)
{
    printf("Query: [start] [end] [numSteps] [funcId]\n");

    size_t numRead = scanf("%lf %lf %zu %zu", start, end, numSteps, funcId);

    return (numRead == 4 && *end >= *start && *numSteps > 0 && *funcId < NUM_FUNCS);
}

int main(void)
{
    // Set up signal handler for SIGCHLD
    signal(SIGCHLD, waitChild);

    double rangeStart;
    double rangeEnd;
    size_t numSteps;
    size_t funcId;

    while (getValidInput(&rangeStart, &rangeEnd, &numSteps, &funcId))
    {
        if (numChildren >= MAX_CHILDREN)
        {
            // Wait for a child to finish before accepting new input
            while (numChildren >= MAX_CHILDREN)
            {
                pause();
            }
        }

        pid_t childPid = fork();

        if (childPid == 0)
        {
            // Child process
            double result = integrateTrap(FUNCS[funcId], rangeStart, rangeEnd, numSteps);
            printf("The integral of function %zu in range %g to %g is %.10g\n", funcId, rangeStart, rangeEnd, result);
            exit(0);
        }
        else if (childPid > 0)
        {
            // Parent process
            numChildren++;
        }
        else
        {
            perror("Fork failed");
            exit(1);
        }
    }

    // Wait for all child processes to finish
    // while (numChildren > 0)
    // {
    //     pause();
    // }

    exit(0);
}
