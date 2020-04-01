#include "headers.h"
#include "PriorityQueue.h"
#include "Queue.h"
#include "PCB.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>


//enum algorithm chosenAlg;
void resumeProcess(PCB* processPCB, FILE* outLogFile, bool silent);
void startProcess(PCB* processPCB, FILE* outLogFile);
void stopProcess(PCB* processPCB, FILE* outLogFile, bool silent);
void finishProcess(PCB* processPCB, FILE* outLogFile);
void handler(int signum);
//void SRTN(FILE* outLogFile);
void HPF(FILE* outLogFile);

bool succesful_exit_handler = false;   //global variable to store the handler result of exit code
//bool finish_scheduler = false;         //global variable to store if the scheduler should stop (No other processes)

int main(int argc, char * argv[])
{

    // Establish communication with the clock module
    initClk();

    // Calling the written handler when the child exits 
    signal(SIGCHLD, handler);
  
    // Open an output file for the scheduler log (in the write mode)
    FILE* outLogFile = (FILE *) malloc(sizeof(FILE));
    outLogFile = fopen("SchedulerLog.txt", "w");
    if (outLogFile == NULL) {
        printf("Could not open output file for scheduler log.\n");
    }

    // Reading the main arguments 
    int quantum;
    //char shedalg[5];
    char *shedalg = NULL;
    if (argc == 2) {
        
        // Get the quantum needed for round robin algorithm 
        quantum = atoi(argv[1]);
        printf("Quantum %d.\n", quantum);

        // Get the chosen algorithm 
        shedalg = argv[0];
        printf("Alg: %s\n", shedalg); 
    }
    // Deciding which algorithm that the process generator sent 
    //if (strcmp(shedalg,"HPF") != 0)
    //{
        //printf("finish schedulerrrrrrrrr %d   ", finish_scheduler);
        HPF(outLogFile);  
    
    //}


    // Close the output log file
    fclose(outLogFile);
    
    // Upon termination, release resources of communication with the clock module
    
    //destroyClk(false);

    return 0;
}

void resumeProcess(PCB* processPCB, FILE* outLogFile, bool silent) {

    // Send a continue signal to the process
    kill(processPCB->pid, SIGCONT);

    // Calculate and update the process waiting time
    int currTime = getClk();
    processPCB->waitingTime = (currTime - processPCB->arrivalTime) - (processPCB->runTime - processPCB->remainingTime);

    // Print the "resuming" line in the output log file
    if (!silent) {
        fprintf(outLogFile, "At time %d process %d resumed arr %d total %d remain %d wait %d\n", currTime, processPCB->id, processPCB->arrivalTime, processPCB->runTime, processPCB->remainingTime, processPCB->waitingTime);
    }
}

void startProcess(PCB* processPCB, FILE* outLogFile) {

    // Create the process
    int pid = fork();

    if (pid == -1)
        perror("Error in fork. Could not start process.\n");

    else if (pid == 0)
    {
        char str[100];
        sprintf(str, "%d", processPCB->runTime);
        char *argv[] = { "./process.out", str};
        execve(argv[0], &argv[0], NULL);
    }
    else {
        int currTime = getClk();

        // Update the process PCB fields as appropriate
        processPCB->pid = pid;
        processPCB->startTime = currTime;
        processPCB->remainingTime = processPCB->runTime;
        processPCB->waitingTime = processPCB->arrivalTime - currTime;
        //printf("Process created successfully.\n");

        // Print the "starting" line in the output log file
        fprintf(outLogFile, "At time %d process %d started arr %d total %d remain %d wait %d\n", currTime, processPCB->id, processPCB->arrivalTime, processPCB->runTime, processPCB->remainingTime, processPCB->waitingTime);

    }
}

void stopProcess(PCB* processPCB, FILE* outLogFile, bool silent) {

    //send a stop signal to the process
    kill(processPCB->pid, SIGSTOP);

    // Calculate and update the process remaining time
    int currTime = getClk();
    processPCB->remainingTime = (processPCB->runTime) -  (currTime - processPCB->arrivalTime - processPCB->waitingTime);

    // Print the "starting" line in the output log file
    if(!silent) {
        fprintf(outLogFile, "At time %d process %d stopped arr %d total %d remain %d wait %d\n", currTime, processPCB->id, processPCB->arrivalTime, processPCB->runTime, processPCB->remainingTime, processPCB->waitingTime);
    }
}

void finishProcess(PCB* processPCB, FILE* outLogFile)
{
    int currTime = getClk();
    fprintf(outLogFile, "At time %d process %d finished arr %d total %d remain %d wait %d\n", currTime, processPCB->id, processPCB->arrivalTime, processPCB->runTime, processPCB->remainingTime, processPCB->waitingTime);
    processPCB = NULL;
    succesful_exit_handler = false;
}

void handler(int signum) {
    
    int pid, stat_loc;
    printf("\nfrom handler my Id: %d\n",getpid() ); 

    printf("Child has sent a SIGCHLD signal #%d\n",signum);

    pid = wait(&stat_loc);
    if(WIFEXITED(stat_loc))
    {
        succesful_exit_handler = true;
    }
    else
    {
        succesful_exit_handler = false;
    }
  
}

/*
void SRTN(FILE* outLogFile) {

    PNode** PQueueHead = NULL;
    *PQueueHead = NULL;
    struct msgbuff* tempBuffer;
    PCB* tempPCB;
    //bool silence = false; //////////

    while (1) {  ////te2felllll + silent in file or not

        if (isEmpty(PQueueHead)) {
            receiveMsg(1, tempBuffer);
            tempPCB = tempBuffer->data;
            if (tempPCB->pid == -10)
            {
                break;
            }
            push(PQueueHead, tempPCB, tempPCB->remainingTime);

            while(receiveMsg(0, tempBuffer)) {
                tempPCB = tempBuffer->data;
            if (tempPCB->pid == -10)
            {
                break;
            }
                push(PQueueHead, tempPCB, tempPCB->remainingTime);
            }
        }

        PCB* currProcessPCB = (PCB *) malloc(sizeof(PCB));      
        pop(PQueueHead, currProcessPCB);
        
        if (currProcessPCB->pid == -5) {
            startProcess(currProcessPCB, outLogFile);
        }
        else {
            resumeProcess(currProcessPCB, outLogFile, 0);
        }
        
        receiveMsg(1, tempBuffer);

        if (succesful_exit_handler) {
            finishProcess(currProcessPCB, outLogFile); /////////////////////// set successful exit handler
        }
        else {
            stopProcess(currProcessPCB, outLogFile, 0);
            tempPCB = tempBuffer->data;
            push(PQueueHead, tempPCB, tempPCB->remainingTime);
            push(PQueueHead, currProcessPCB, currProcessPCB->remainingTime);
        }

        while(receiveMsg(0, tempBuffer)) {
            tempPCB = tempBuffer->data;
            push(PQueueHead, tempPCB, tempPCB->remainingTime);
        }

        /*
        while (receiveMsg(0, tempBuffer)) { ////while hena en fi msgs gat, w msh elseeeeeeeeee
            tempPCB = tempBuffer->data;

            int currTime = getClk();
            int currProcessRemTime = (currProcessPCB->runTime) -  (currTime - currProcessPCB->arrivalTime - currProcessPCB->waitingTime);

            if ((tempPCB->remainingTime < currProcessRemTime) && !preemptionFlag) {
                preemptionFlag = true;
                stopProcess(currProcessPCB, outLogFile, 0);         
            }
            
            push(PQueueHead, tempPCB, tempPCB->remainingTime);
        }
        

        //sleep(currProcessPCB->remainingTime); ///////////////////////

        //silence = false;
        

        silence = false;        
        
        if (!preemptionFlag) {
            stopProcess(currProcessPCB, outLogFile, 1);  
            push(PQueueHead, currProcessPCB, currProcessPCB->remainingTime);
        }
        */
   /* } 
}*/


void HPF(FILE* outLogFile){
    
    PNode* ReadyQueue = NULL;
    struct msgbuff message;
    PCB* temp_process_pcb = NULL;
    PCB* ready_process_pcb = NULL;
    //printf("finish scheduler %d   ", finish_scheduler);
    while (1)
    {
        //printf("finish scheduler %d   ", finish_scheduler);
        if (isEmpty(&ReadyQueue))
        {
            //printf("finish scheduler %d   ", finish_scheduler);
            /*if (finish_scheduler == true)
            {
                printf("HPPPPPPPPPFFFFFFF");
                break;
            }*/
            receiveMsg(1,message);
            temp_process_pcb = &(message.data);
            /*if (temp_process_pcb->pid == -10)
            {
                printf("true1/n");
                finish_scheduler = true;
                break;
            }*/
            push(&ReadyQueue, temp_process_pcb, temp_process_pcb->priority);
        }
        
        while (receiveMsg(0,message))
        {
            temp_process_pcb = &(message.data);
            /*if (temp_process_pcb->pid == -10)
            {
                printf("true1/n");
                finish_scheduler = true;
            }*/
            push(&ReadyQueue, temp_process_pcb, temp_process_pcb->priority);
        }
        
        if (!isEmpty(&ReadyQueue))
        {
            pop(&ReadyQueue,ready_process_pcb);
            startProcess(ready_process_pcb, outLogFile);
            sleep(ready_process_pcb->runTime);
            
            while (!succesful_exit_handler)
            {
                stopProcess(ready_process_pcb, outLogFile,1);
                resumeProcess(ready_process_pcb, outLogFile,1);
                sleep(ready_process_pcb->remainingTime);
            }
            finishProcess(ready_process_pcb, outLogFile);
        }  
    }  
}