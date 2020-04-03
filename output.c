#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include "PCB.h"
#include <math.h>
#include "Queue.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>



//Creating file fn :
FILE *CreateFile() 
{
    return fopen("Output.txt", "w");  // allocates a FILE object and returns a pointer to it
}

// Initialize a queue to store all WTA for all processes :
    Queue WTAQueue; 
    
    queueInit(&WTAQueue, sizeof(int));

    // Create dynamic memory for PCBs
    int* WTApointer = (int *) malloc(sizeof(int)); 


//Global variables :
    int counter=1;
    int total_WTA;
    int AWTA;
    int total_waiting_time;
    int total_proceesing_time;
    int Avg_waiting;
    int SD_numerator;
    int SD;

void outfile_calculation(PCB* processPCB,int total_time) {

    FILE *fp=CreateFile();
    //Checking for correctness of opening the file 
     if(fp == NULL)
    {
        printf("Error! Could not open file\n");    
        exit(1);             
    }
    //Accessing members of PCB :
    int Arrival_Time=processPCB->arrivalTime;
    int Finish_Time=processPCB->finishTime;
    int Run_Time=processPCB->runTime;
    int Waiting_Time=processPCB->waitingTime;
    int turn_around_time=Finish_Time-Arrival_Time;

    //Total Weighted turn around time
    int Calculated_WTA=turn_around_time/Run_Time;
    //WTApointer...I will use this pointer in calculating standard deviation :
    WTApointer=&(Calculated_WTA);
    total_WTA=total_WTA+Calculated_WTA;
    AWTA=total_WTA/counter;
    fprintf(fp,"Avg WTA = %d \n",AWTA);

    //Total Waiting_Time
    total_waiting_time=total_waiting_time+Waiting_Time;
    Avg_waiting=total_waiting_time/counter;
    fprintf(fp,"Avg Waiting = %d \n",Avg_waiting);

    //CPU_utilization :
    total_proceesing_time=total_proceesing_time+Run_Time;
    int cpu_utilization=(total_proceesing_time/total_time)*100;
    fprintf(fp, "CPU  utilization: %d %% \n",cpu_utilization);
    
    //incrementing the counter 
    counter++;
    fprintf("counter = %d \n",counter);
    //Closing the file

    fclose(fp); //Don't forget to close the file when finished 
    fp=NULL;
    
}

void Standard_deviation()
{   
    //Enqueue WTA in WTAQueue :
    enqueue(&WTAQueue ,WTApointer);
    //Dequeuing from WTAQueue to calculate standard deviation :
    int* dequeue_pointer = NULL;
    while (getQueueSize(&WTAQueue) != 0) 
        {
        int val= dequeue(&WTAQueue,dequeue_pointer);
        //print("pointer value %d \n",*(dequeue_pointer))
        
        pow(val-AWTA, 2);
        SD_numerator +=val;

    SD= sqrt(SD_numerator/counter);
    //need to print in file
    print("Std WTA = %d \n",SD);

}


 int main()
 {
 return 0;
 }


//standard deviation
//handling el useful time