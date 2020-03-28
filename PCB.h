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
    enum {STARTED, RESUMED, STOPED, FINISHED} state;

}   PCB;



// I think that i will add here initilization for this struct also 
// may be like a function || will figure it out later