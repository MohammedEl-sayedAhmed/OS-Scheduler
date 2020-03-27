#include "headers.h"
#include "PCB.h"

void resumeProcess(PCB* processPCB, FILE* outLogFile);
void startProcess(PCB* processPCB, FILE* outLogFile);

int main(int argc, char * argv[])
{
    // Establish communication with the clock module
    initClk();
    
    // Open an output file for the scheduler log (in the write mode)
    FILE* outLogFile = (FILE *) malloc(sizeof(FILE));
    outLogFile = fopen("SchedulerLog.txt", "w");
    if (outLogFile == NULL) {
        printf("Could not open output file for scheduler log.\n");
    }



    // Close the output log file
    fclose(outLogFile);
    // Upon termination, release resources of communication with the clock module
    destroyClk(true);
}

void resumeProcess(PCB* processPCB, FILE* outLogFile) {

    // Send a continue signal to the process
    kill(processPCB->pid, SIGCONT);

    // Calculate and update the process waiting time
    int currTime = getClk();
    processPCB->waitingTime = (currTime - processPCB->arrivalTime) - (processPCB->runTime - processPCB->remainingTime);

    // Print the "resuming" line in the output log file
    fprintf(outLogFile, "At time %d process %d resumed arr %d total %d remain %d wait %d\n", currTime, processPCB->id, processPCB->arrivalTime, processPCB->runTime, processPCB->remainingTime, processPCB->waitingTime);

}

void startProcess(PCB* processPCB, FILE* outLogFile) {

    // Create the process
    int pid = fork();

    if (pid == -1)
        perror("Error in fork. Could not start process.\n");

    else if (pid == 0)
    {
        char str[100];
        sprintf(str, "%d", processPCB->runTime);
        char *argv[] = { "./process.out", str};
        execve(argv[0], &argv[0], NULL);
    }
    else {
        int currTime = getClk();

        // Update the process PCB fields as appropriate
        processPCB->pid = pid;
        processPCB->startTime = currTime;
        processPCB->remainingTime = processPCB->runTime;
        processPCB->waitingTime = processPCB->arrivalTime - currTime;
        //printf("Process created successfully.\n");

        // Print the "starting" line in the output log file
        fprintf(outLogFile, "At time %d process %d started arr %d total %d remain %d wait %d\n", currTime, processPCB->id, processPCB->arrivalTime, processPCB->runTime, processPCB->remainingTime, processPCB->waitingTime);

    }
}