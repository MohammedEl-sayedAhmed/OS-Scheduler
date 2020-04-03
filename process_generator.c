#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include "headers.h"
#include "PCB.h"
#include "Queue.h"

pid_t schedulerPID, clockPID;
pid_t createClock();
pid_t createScheduler(char * const * argv);
void readInputFile(Queue* arrivedProcessesQueue);
void sendProcessAtAppropTime(Queue* arrivedProcessesQueue);
void clearResources(int signum);


int main(int argc, char * argv[])
{
    // Specify handler to SIGINT so that resources are cleared upon interruption
    signal(SIGINT, clearResources);


    // Initialize a queue to store all processes
    Queue arrivedProcessesQueue;
    queueInit(&arrivedProcessesQueue, sizeof(PCB));
    
    // Read all incoming processes from the input file
    readInputFile(&arrivedProcessesQueue);


    // Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    // SRTN: Shortest Remaining Time Next, HPF: Highest Priority First (non permitive), RR: Round Robin
    char  scheduling_algorithm[5];
    int Quantum = 0;
    printf("SRTN, HPF, RR are the available scheduling algorithms; choose one of them: \n");
    scanf ("%s", scheduling_algorithm);

    // Verify user has chosen a valid scheduler algorithm    
    while(strcmp(scheduling_algorithm,"RR")!=0 && strcmp(scheduling_algorithm,"SRTN")!=0 && strcmp(scheduling_algorithm,"HPF")!=0)
    {
        printf ("Error: You must choose one of the provided algorithms only.\n");
        printf("SRTN, HPF, RR are the available scheduling algorithms; choose one of them: \n");
        scanf ("%s", scheduling_algorithm);
    }

    // Ask user to choose the desired quantum in case of RR
    if(strcmp(scheduling_algorithm, "RR") == 0)
    {
        printf ("Enter quantum:\n");
        scanf("%d", &Quantum);
    }
    char QuantumStr[100];
    sprintf(QuantumStr, "%d", Quantum);


    // Create the clock process and establish communication with it
    clockPID = createClock();
    initClk();
    ///////printf("Initialized clock.\n");


    // Initialize a message queue through which processes will be sent to the scheduler
    initMsgQueue();

    // Create the scheduler process and send the chosen algorithm and its parameters to it
    char * param[] = {scheduling_algorithm, QuantumStr , NULL};
    schedulerPID = createScheduler(param);

    printf("before if condition   .\n");
    // Send processes to the scheduler at their appropriate arrival time
    sendProcessAtAppropTime(&arrivedProcessesQueue);

    
    // Wait till scheduler terminates
    int stat;
    pid_t isSched = waitpid(schedulerPID, &stat, 0);
    //pid_t isSched = wait(&stat);
 
    // Check for successful termination of scheduler 
    if ((isSched != schedulerPID) || !(WIFEXITED(stat))) {
        printf("Signal received from scheduler %d.\n", WEXITSTATUS(stat));

        if (WIFSIGNALED(stat)) {
            psignal(WTERMSIG(stat), "Exit signal");         
        }
    }
    printf("Scheduler exited with exit code: %d.\n", WEXITSTATUS(stat)); 

    // Clear all resources and terminate the whole system 
    raise(SIGINT);
}

//////////////////////////////////////////////

// Create clock
pid_t createClock(){

    // Fork clock
    pid_t clockPID = fork();

    if (clockPID == -1){
        perror("Error in forking clock.\n");
        exit(1);
    }
    else if (clockPID == 0){

        printf("Creating clock.\n");

        // Run clk.out
        char *argv[] = {"./clk.out", 0};
        execve(argv[0], &argv[0], NULL);
    }
    return clockPID;  
}

// Create scheduler
pid_t createScheduler(char * const * argv){

    // Forking scheduler
    pid_t schedulerPID = fork();

    if (schedulerPID == -1){
        perror("Error in forking scheduler.\n");
        exit(1);
    }
    else if (schedulerPID == 0){

        // Run scheduler.out
        printf("Creating scheduler.\n");
        execv("./Scheduler.out", argv); // argv is the list of arguments to pass (scheduling algorithm + necessary parameters)
    }
    return schedulerPID;  
}

// Read the input file
void readInputFile(Queue* arrivedProcessesQueue)
{
    // Open the input file in a read mode
    FILE *inputFile; 
    inputFile = fopen("processes.txt", "r");
    // Assign the lines in the txt file to intputFileLine
    char intputFileLine[20];

    // While the file is opened
   while (!feof(inputFile) ) {

        // Obtain the first character
        intputFileLine[0] = fgetc(inputFile); 
        
        // If it is not #, then it is a process
        if (intputFileLine[0] != '#'){

            // Create process PCB dynamically
            PCB * myNewPCB = (PCB *) malloc(sizeof(PCB)); 
          
            // Save the ID first, because there is a bug that makes the ID = 0 many times so we want to check for it
            // Convert the string to int
            myNewPCB->id = atoi(intputFileLine); 

            if (myNewPCB->id == 0){
                // To skip the 0 process id that comes in between
                continue;
            }

            // Read the process data and save it properly to the PCB struct object
            fscanf(inputFile , "%d %d %d", &myNewPCB->arrivalTime, &myNewPCB->runTime , &myNewPCB->priority);

            // Initializing the rest of the members
            PCBinit(myNewPCB);

            // Enqueue myNewPCB in arrivedProcessesQueue
            enqueue(arrivedProcessesQueue , myNewPCB);
        }
        else{
            // The commented line in the input file (starting with #)
            char notProcessData[200];
            fgets(notProcessData, sizeof(notProcessData), inputFile);  
        }
    }
    // Close the file
    fclose(inputFile);
}

// Send processes to the scheduler at their arrival time
void sendProcessAtAppropTime (Queue* arrivedProcessesQueue) {


    // While there are still processes
	while (getQueueSize(arrivedProcessesQueue) != 0) {

        PCB nextProcess;

        // Dequeue next process
        dequeue(arrivedProcessesQueue, &nextProcess);

        // Calculate time to next process
		int currTime = getClk();
		int sleepTime = nextProcess.arrivalTime - currTime;
        
        ///////printf("Will sleep for %d seconds.", sleepTime);
		sleep(sleepTime);

		///////currTime = getClk();
        ///////printf("Sending process at %d.", currTime);

        // Send the process to the scheduler
        bool sentStatus = sendMsg(nextProcess);
        if(!sentStatus) {
            printf("Couldn't send message to scheduler.\n");
        }
	}
    

    // Create a flag PCB (pid = -10) to send to the scheduler as an inidcation that no other processes will be sent
    PCB flagProcess;
    flagProcess.pid = -10;

    // Send the flag PCB to the scheduler
    bool sentStatus = sendMsg(flagProcess);
    if(!sentStatus) {
        printf("Couldn't send message to scheduler.\n");
    }
}

// Clears all resources and terminates the whole system either in case of successful termination or interruption
void clearResources(int signum)
{
    printf("Resources are cleared now..\n");

    // Interrupt scheduler in case it is still running
    kill(schedulerPID, SIGINT);

    // Interrupt clock
    kill(clockPID, SIGINT);

    // Delete message queue between process generator and scheduler
    destroyMsgQueue();

    // Release resources of communication with the clock module, all other releases, and terminates the whole system
    destroyClk(true);
    exit(0);
}