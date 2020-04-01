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

void readInputFile(Queue* arrivedProcessesQueue);

int main() {

    Queue arrivedProcessesQueue;
/*
    PCB myPCB2;
    myPCB2.arrivalTime = 5;
    myPCB2.id = 8;
    myPCB2.priority = 7;
    myPCB2.runTime = 9;
*/
    queueInit(&arrivedProcessesQueue, sizeof(PCB));
    readInputFile(&arrivedProcessesQueue);

    PCB deqPCB;
    PCB peekPCB;
    
    dequeue(&arrivedProcessesQueue, &deqPCB);


    //dequeue(&q, &deqPCB);
    printf("The deqPCB arrival time is %d\n", deqPCB.arrivalTime);
    printf("The deqPCB ID is %d\n", deqPCB.id);
    printf("The deqPCB priority is %d \n", deqPCB.priority);
    printf("The deqPCB run time is %d \n\n\n", deqPCB.runTime);

    dequeue(&arrivedProcessesQueue, &deqPCB);


    //dequeue(&q, &deqPCB);
    printf("The deqPCB arrival time is %d\n", deqPCB.arrivalTime);
    printf("The deqPCB ID is %d\n", deqPCB.id);
    printf("The deqPCB priority is %d \n", deqPCB.priority);
    printf("The deqPCB run time is %d \n\n\n", deqPCB.runTime);

    dequeue(&arrivedProcessesQueue, &deqPCB);


    //dequeue(&q, &deqPCB);
    printf("The deqPCB arrival time is %d\n", deqPCB.arrivalTime);
    printf("///////////The deqPCB ID is %d\n", deqPCB.pid);
    printf("The deqPCB priority is %d \n", deqPCB.priority);
    printf("////////The deqPCB run time is %d \n\n\n", deqPCB.remainingTime);

    dequeue(&arrivedProcessesQueue, &deqPCB);


    //dequeue(&q, &deqPCB);
    printf("The deqPCB arrival time is %d\n", deqPCB.arrivalTime);
    printf("The deqPCB ID is %d\n", deqPCB.id);
    printf("The deqPCB priority is %d \n", deqPCB.priority);
    printf("The deqPCB run time is %d \n\n\n", deqPCB.runTime);

    dequeue(&arrivedProcessesQueue, &deqPCB);


    //dequeue(&q, &deqPCB);
    printf("The deqPCB arrival time is %d\n", deqPCB.arrivalTime);
    printf("The deqPCB ID is %d\n", deqPCB.id);
    printf("The deqPCB priority is %d \n", deqPCB.priority);
    printf("The deqPCB run time is %d \n\n\n", deqPCB.runTime);

    
    return 0;
}


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

            ///////////////me7tageen n initialize ba2eet el7agat kamannnnnnn
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
