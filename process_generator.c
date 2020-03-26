#include <stdio.h>
#include <unistd.h>
#include "headers.h"
#include "PCB.h"
#include "Queue.h"


void clearResources(int);

void readInputFile(Queue* arrivedProcessesQueue);

pid_t createScheduler(char * const * argv);
pid_t createClock();
void sendProcessAtAppropTime (Queue* arrivedProcessesQueue,PCB* arrivedProcess );



int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    Queue* arrivedProcessesQueue;
    
    // 1. Read the input files.
    readInputFile(arrivedProcessesQueue);

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.

    //SRTN shortest remaining time next , HPF -heighest priority first -non permitive- , RR Round Robin

    char  scheduling_algorithm[5];
    int Quantum;
    
    printf("SRTN,HPF,RR are the available scheduling algorithms ,choose one of them \n");
    scanf ("%s",scheduling_algorithm);
    
    while(strcmp(scheduling_algorithm,"RR")!=0 && strcmp(scheduling_algorithm,"STRN")!=0 && strcmp(scheduling_algorithm,"HPF")!=0)
    {
        printf ("Error ,You must choose one of the provided algorithms only \n");
        printf("SRTN,HPF,RR are the available scheduling algorithms ,choose one of them \n");
        scanf ("%s",scheduling_algorithm);
    }
    if(strcmp(scheduling_algorithm, "RR") == 0)
    {
        printf ("Enter Quantum \n");
        scanf("%d",&Quantum);
    }


    // 3. Initiate and create the scheduler and clock processes.
    // define the argv array 
    char * argv[] = {"scheduler.out", scheduling_algorithm, Quantum , NULL}; 
    pid_t schedulerPID = createScheduler(argv);
    createClock();

    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    PCB* arrivedProcess;

    // 6. Send the information to the scheduler at the appropriate time.
    sendProcessAtAppropTime(arrivedProcessesQueue,arrivedProcess);

    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
}


void readInputFile(Queue* arrivedProcessesQueue)
{
    FILE *inputFile; 
    inputFile = fopen("processes.txt", "r"); // open the input file in a read mode
    char intputFileLine[20]; // assign the lines in the txt file to intputFileLine

   while (!feof(inputFile) ) {

        intputFileLine[0] = fgetc(inputFile); // fgetc() is used to obtain input from a file single character at a time
        
        if (intputFileLine[0] != '#'){ // if it is not #, then it is a process

            // this is a process -- save it in whatever stuff (will be a queue later)
            PCB * myNewProcess = (PCB *) malloc(sizeof(PCB)); // malloc >> memory allocation, it is like new in c++
          
            // save the ID first, because there is a bug that makes the ID= 0 many times so we want to check for it
            // Also use atoi; to convert the string to int -- like %d in fscanf 
            myNewProcess->id = atoi(intputFileLine); 

            if (myNewProcess->id == 0){
                continue; // to skip the 0 process id that comes in between
            }

            fscanf(inputFile , "%d %d %d", &myNewProcess->arrivalTime, &myNewProcess->runTime , &myNewProcess->priority); // read the process data and save it proberly to the pcb struct object

            /*printf("The ID is %d\n", myNewProcess->id);
            printf("The arrival time is %d\n", myNewProcess->arrivalTime);
            printf("The run time is %d \n", myNewProcess->runTime);
            printf("The priority is %d \n\n\n", myNewProcess->priority);*/
            
            // enqueue myNewProcess in arrivedProcessesQueue
            //Queue arrivedProcessesQueue;
            queueInit(&arrivedProcessesQueue, sizeof(PCB));
            enqueue(&arrivedProcessesQueue , myNewProcess);
        }
        else{ // malhash lazma, just 7antafa 

            char notProcessData[200];
            fgets(notProcessData, sizeof(notProcessData), inputFile);  
        }
    } 
    fclose(inputFile);
}


pid_t createScheduler(char * const * argv){

    pid_t schedulerPID = fork();

    if (schedulerPID == -1){

        perror("Error in scheduler fork\n");
        exit(1);
    }

    else if (schedulerPID == 0){

         printf("creating scheduler process\n");
        int forkResult;
        forkResult = execv("./scheduler.out", argv); // argv will be the paramter list defined in the main
        
        if (forkResult == -1){
            printf("Failed to create scheduler process\n");
        }
        else if(forkResult == 0){
            printf("Scheduler process created successfully!! \n");
        }
    }
    return schedulerPID;  
}


pid_t createClock(){

    pid_t clockPID;
    
    if (clockPID == -1){

        perror("Error in Clock fork\n");
        exit(1);
    }
    else if (clockPID == 0){

        printf("creating clock process\n");

        int forkResult;
        forkResult = execvp("./clk.out",NULL);
        if (forkResult == -1){
            printf("Failed to create clock process\n");
        }
        else if (forkResult == 0){
            printf("Clock process created successfully!! \n");
        }
    }
    return clockPID;
}



void sendProcessAtAppropTime (Queue* arrivedProcessesQueue,PCB* arrivedProcess ){

	while (getQueueSize(arrivedProcessesQueue) != 0){

		int currTime = getClk();
        printf("current time is %d\n", currTime);

		int currProcessArrTime = arrivedProcess->arrivalTime;
		int sleepTime = currProcessArrTime - currTime;

		sleep(sleepTime);

	}
}

    