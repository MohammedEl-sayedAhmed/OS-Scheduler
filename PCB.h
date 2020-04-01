#pragma once 
#include <stdlib.h>

typedef struct 
{
    
    // Process from text file
    int id;
    int arrivalTime;
    int runTime;
    int priority;

    // PCB data
    pid_t pid;
    int startTime;
    int remainingTime;
    int waitingTime;
    int finishTime;
    enum {MAGATSH, RUNNING, WAITING, FINISHED} state;

}   PCB;

// Initializing PCB after reading it from the input file
void PCBinit(PCB *newlyReadPCB) {

    // Set pid to -5 to indicate that process has not been forked yet
    newlyReadPCB->pid = -5;
    newlyReadPCB->startTime = -1;
    newlyReadPCB->remainingTime = newlyReadPCB->runTime;
    newlyReadPCB->waitingTime = 0;
    newlyReadPCB->finishTime = -1;
    newlyReadPCB->state = MAGATSH;

}