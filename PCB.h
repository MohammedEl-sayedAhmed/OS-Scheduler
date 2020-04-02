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

void equate(PCB *from, PCB *to) {
    to->id = from->id;
    to->arrivalTime = from->arrivalTime;
    to->runTime = from->runTime;
    to->priority = from->priority;

    // PCB data
    to->pid = from->pid;
    to->remainingTime = from->remainingTime;
    to->startTime = from->startTime;
    to->waitingTime = from->waitingTime;
    to->finishTime = from->finishTime;
    to->state = from->state;
}