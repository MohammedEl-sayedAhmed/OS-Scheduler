#include "headers.h"
#include <time.h>

// Process running and remaining times
int runningtime, remainingtime;

int main(int argc, char * argv[])
{
    //printf("Inside process.\n");
 
    // Establish communication with the clock module
    initClk();

    // Check if only one other argument (running time) is passed to the process other than its name
    if (argc == 1) {

        // Set the process running time to equal that passed to it
        runningtime = atoi(argv[0]);
        ///////printf("Process intended running time is: %d.\n", runningtime);

        // Initialize the remaining time with the running time
        remainingtime = runningtime;
    }
    else {
        // If a wrong number of arguments is passed, print out this message to the user and return with 1
        printf("Expected number of passed arguments is 1.\n");
        exit(1);
    }

    // As long as the process has not finished its intended running time, keep running
    while (remainingtime > 0)
    {
        // Get the total number of clocks the process took being processed so far
        clock_t totalClocks = clock();
        ///////printf("Total processing time taken: %li second(s).\n", (totalClocks/CLOCKS_PER_SEC));

        // Calculate the new remaining time
        remainingtime = runningtime - ((int) (totalClocks/CLOCKS_PER_SEC));
        ///////printf("Remaining time: %d second(s).\n", remainingtime);
    }

    //printf("Process finished.\n");

    
    // Release resources of communication with the clock module
//    destroyClk(false);
    kill(getppid(), SIGUSR1);
    return 0;
}
