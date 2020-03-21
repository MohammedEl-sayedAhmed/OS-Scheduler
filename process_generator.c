#include <stdio.h>

#include "headers.h"
#include "PCB.h"


void clearResources(int);
void readInputFile();
int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    readInputFile();
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    // 3. Initiate and create the scheduler and clock processes.
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    printf("current time is %d\n", x);
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
}

//1. Read the input files.
void readInputFile()
{
    FILE *inputFile;
    inputFile = fopen("processes.txt", "r");
    char processData[20];

   while (!feof(inputFile) ) {

        processData[0] = fgetc(inputFile); // fgetc() is used to obtain input from a file single character at a time
        
        if (processData[0] != '#'){ // if it is not #, then it is a process

            // this is a process -- save it in whatever stuff (will be a queue later)
            PCB * myNewPCB = (PCB *) malloc(sizeof(PCB)); // malloc >> memory allocation, it is like new in c++
          
            // save the ID first, because there is a bug that makes the ID= 0 many times so we want to check for it
            // Also use atoi; to convert the string to int -- like %d in fscanf 
            myNewPCB->id = atoi(processData); 

            if (myNewPCB->id == 0){
                continue; // to skip the 0 process id that comes in between
            }

            fscanf(inputFile , "%d %d %d", &myNewPCB->arrivalTime, &myNewPCB->runTime , &myNewPCB->priority); // read the process data and save it proberly to the pcb struct object

            printf("The ID is %d\n", myNewPCB->id);
            printf("The arrival time is %d\n", myNewPCB->arrivalTime);
            printf("The run time is %d \n", myNewPCB->runTime);
            printf("The priority is %d \n\n\n", myNewPCB->priority);


        }
        else{ // malhash lazma, just 7antafa 

            char notProcessData[200];
            fgets(notProcessData, sizeof(notProcessData), inputFile);  
        }
    } 
}


    