#pragma once 

#include "ProcessFromInput.h"
#include <stdlib.h>

typedef struct 
{
    ProcessFromInput newArrivedProcess;
    pid_t pid;
    int startTime;
    int remainingTime;
    int waitingTime;
    int finishTime;
 //   enum {STARTED, RESUMED, STOPED, FINISHED};

}   PCB;



// I think that i will add here initilization for this struct also 
// may be like a function || will figure it out later