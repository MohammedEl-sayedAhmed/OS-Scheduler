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

//void clearResources(int signum);
void readInputFile(Queue* arrivedProcessesQueue);

pid_t createScheduler(char * const * argv);
pid_t createClock();
void sendProcessAtAppropTime (Queue* arrivedProcessesQueue);

//////////////////////////////
void end(int signum) {
    printf("inside end\n");
    destroyClk(true);
    exit(0);
}

int main(int argc, char * argv[])
{
    signal(SIGINT, end);
//    signal(SIGINT, clearResources); ///////mafrod n kill scheduler w kollo fiha sa7?
    // TODO Initialization
    Queue arrivedProcessesQueue;
    queueInit(&arrivedProcessesQueue, sizeof(PCB));
    
    // 1. Read the input files.
    readInputFile(&arrivedProcessesQueue);

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.

    //SRTN: Shortest Remaining Time Next, HPF: Highest Priority First (non permitive), RR: Round Robin
    char  scheduling_algorithm[5];
    int Quantum = 0;
    
    printf("SRTN, HPF, RR are the available scheduling algorithms; choose one of them: \n");
    scanf ("%s", scheduling_algorithm);
    
    while(strcmp(scheduling_algorithm,"RR")!=0 && strcmp(scheduling_algorithm,"SRTN")!=0 && strcmp(scheduling_algorithm,"HPF")!=0)
    {
        printf ("Error: You must choose one of the provided algorithms only.\n");
        printf("SRTN, HPF, RR are the available scheduling algorithms; choose one of them: \n");
        scanf ("%s", scheduling_algorithm);
    }

    if(strcmp(scheduling_algorithm, "RR") == 0)
    {
        printf ("Enter quantum:\n");
        scanf("%d", &Quantum);
    }

    char QuantumStr[100];
    sprintf(QuantumStr, "%d", Quantum);


    // 3. Initiate and create the scheduler and clock processes.
    // define parameter list

    initMsgQueue();

    pid_t clockPID = createClock();

    char * param[] = {scheduling_algorithm, QuantumStr , NULL};
    pid_t schedulerPID = createScheduler(param);


    // 4. Use this function after creating the clock process to initialize clock
    initClk();

    //sleep(5);
    printf("Initialized clock.\n");

    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.

    // 6. Send the information to the scheduler at the appropriate time.
    sendProcessAtAppropTime(&arrivedProcessesQueue);
    // send to scheduler
    //Send_msg(struct msgbuff message); // will be fixed 


    // 7. Clear clock resources
    destroyClk(true);
    printf("Destroyed clock.\n");
    kill(clockPID, SIGINT);
    kill(schedulerPID, SIGINT);
    return 0;
}

/*
//TODO Clears all resources in case of interruption
//clearResources function :
void clearResources(int signum)
{
    printf("Resources are cleared now..\n");
    int x = msgget(key,0644);
    msgctl(x, IPC_RMID, (struct msqid_ds *) 0);
    exit(0);
}
*/
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

            /*
            printf("The ID is %d\n", myNewPCB->id);
            printf("The arrival time is %d\n", myNewPCB->arrivalTime);
            printf("The run time is %d \n", myNewPCB->runTime);
            printf("The priority is %d \n\n\n", myNewPCB->priority);
            */        

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

// Create scheduler
pid_t createScheduler(char * const * argv){

    pid_t schedulerPID = fork();

    if (schedulerPID == -1){
        perror("Error in forking scheduler.\n");
        exit(1);
    }
    else if (schedulerPID == 0){

        printf("Forking scheduler.\n");
        execv("./testSleep1.out", argv); // argv is the list of arguments to pass (scheduling algorithm + necessary parameters)
    }
    return schedulerPID;  
}

// Create clock
pid_t createClock(){

    pid_t clockPID = fork();

    if (clockPID == -1){
        perror("Error in forking clock.\n");
        exit(1);
    }
    else if (clockPID == 0){

        printf("Forking clock.\n");

        char *argv[] = {"./clk.out", 0};
        execve(argv[0], &argv[0], NULL);
    }
    return clockPID;  
}

// Send processes to the scheduler at their arrival time
void sendProcessAtAppropTime (Queue* arrivedProcessesQueue) {

    ///////printf("Inside sendProcessAtAppropTime.\n");

    // While there are still processes
	while (getQueueSize(arrivedProcessesQueue) != 0) {

        PCB nextProcess;

        // Dequeue next process
        dequeue(arrivedProcessesQueue, &nextProcess);

        // Calculate time to next process
		int currTime = getClk();
		int sleepTime = nextProcess.arrivalTime - currTime;
        
        ///////printf("Will sleep for %d seconds.\n", sleepTime);
		sleep(sleepTime);

		///////currTime = getClk();
        ///////printf("Sending process at %d.\n", currTime);

        // Send the process to the scheduler
        bool sentStatus = sendMsg(nextProcess);
        if(!sentStatus) {
            printf("Couldn't send message to scheduler.\n");
        }
	}
    /////////////////////////////////FLAGGGG LL SCHED 3SHAN YKHALLAS
}


/*
1. send DONEEEEEEEEEEEEEEEEEEEE
2. flag ll sched
3. wait 3la elsched
4. clear
*/