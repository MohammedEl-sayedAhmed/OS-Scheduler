#include "headers.h"
#include "PCB.h"

void resumeProcess(PCB* processPCB, FILE* outLogFile);
void startProcess(PCB* processPCB, FILE* outLogFile);

int main() {

    FILE* outLogFile = (FILE *) malloc(sizeof(FILE));
    outLogFile = fopen("SchedulerLog.txt", "w");
    if (outLogFile == NULL) {
        printf("Could not open output file for scheduler log.\n");
    }

    PCB* processPCB = (PCB *) malloc(sizeof(PCB));

    processPCB->runTime = 10;

    // Create the process
    int pid = fork();

    if (pid == -1)
        perror("Error in fork. Could not start process.\n");

    else if (pid == 0)
    {
        char str[20];
        sprintf(str, "%d", 10);
        char *argv[] = { "./process.out", str, NULL};
        execv(argv[0], &argv[0]);
    }
    else {
//        int currTime = getClk();

        // Update the process PCB fields as appropriate
        processPCB->pid = pid;
        processPCB->startTime = 1;
        processPCB->remainingTime = processPCB->runTime;
        processPCB->waitingTime = 0;
        printf("Process created successfully.\n");

        // Print the "starting" line in the output log file
        fprintf(outLogFile, "At time process %d started arr %d total %d remain %d wait %d\n", processPCB->id, processPCB->arrivalTime, processPCB->runTime, processPCB->remainingTime, processPCB->waitingTime);

    }

    fclose(outLogFile);
    return 0;

}
