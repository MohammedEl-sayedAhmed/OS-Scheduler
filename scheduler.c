#include "headers.h"
#include "PCB.h"

void resumeProcess(PCB* processPCB, FILE* outLogFile);
void startProcess(PCB* processPCB, FILE* outLogFile);
void stopProcess(PCB* processPCB, FILE* outLogFile);
void handler(int signum);

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
    processPCB->waitingTime = (currTime - processPCB->newArrivedProcess.arrivalTime) - (processPCB->newArrivedProcess.runTime - processPCB->remainingTime);

    // Print the "resuming" line in the output log file
    fprintf(outLogFile, "At time %d process %d resumed arr %d total %d remain %d wait %d\n", currTime, processPCB->newArrivedProcess.id, processPCB->newArrivedProcess.arrivalTime, processPCB->newArrivedProcess.runTime, processPCB->remainingTime, processPCB->waitingTime);

}

void startProcess(PCB* processPCB, FILE* outLogFile) {

    // Create the process
    int pid = fork();

    if (pid == -1)
        perror("Error in fork. Could not start process.\n");

    else if (pid == 0)
    {
        char str[100];
        sprintf(str, "%d", processPCB->newArrivedProcess.runTime);
        char *argv[] = { "./process.out", str};
        execve(argv[0], &argv[0], NULL);
    }
    else {
        int currTime = getClk();

        // Update the process PCB fields as appropriate
        processPCB->pid = pid;
        processPCB->startTime = currTime;
        processPCB->remainingTime = processPCB->newArrivedProcess.runTime;
        processPCB->waitingTime = processPCB->newArrivedProcess.arrivalTime - currTime;
        //printf("Process created successfully.\n");

        // Print the "starting" line in the output log file
        fprintf(outLogFile, "At time %d process %d started arr %d total %d remain %d wait %d\n", currTime, processPCB->newArrivedProcess.id, processPCB->newArrivedProcess.arrivalTime, processPCB->newArrivedProcess.runTime, processPCB->remainingTime, processPCB->waitingTime);

    }
}

void stopProcess(PCB* processPCB, FILE* outLogFile) {

    //send a stop signal to the process
    kill(processPCB->pid, SIGSTOP);

    // Calculate and update the process remaining time
    int currTime = getClk();
    processPCB->remainingTime = (processPCB->newArrivedProcess.runTime) -  (currTime - processPCB->newArrivedProcess.arrivalTime - processPCB->waitingTime);

    // Print the "starting" line in the output log file
    fprintf(outLogFile, "At time %d process %d stopped arr %d total %d remain %d wait %d\n", currTime, processPCB->newArrivedProcess.id, processPCB->newArrivedProcess.arrivalTime, processPCB->newArrivedProcess.runTime, processPCB->remainingTime, processPCB->waitingTime);
}

void hanlder(int signum) {
    
    int pid, stat_loc;
    printf("\nfrom handler my Id: %d\n",getpid() ); 

    printf("Child has sent a SIGCHLD signal #%d\n",signum);

    pid = wait(&stat_loc);
    if(WIFEXITED(stat_loc))
        printf("\nA child with pid %d terminated with exit code %d\n", pid, WEXITSTATUS(stat_loc));
    
}