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

enum algorithm {HPF, SRTN, RR};

enum algorithm chosenAlg;
void resumeProcess(PCB* processPCB, FILE* outLogFile, bool silent);
void startProcess(PCB* processPCB, FILE* outLogFile);
void stopProcess(PCB* processPCB, FILE* outLogFile, bool silent);
void finishProcess(PCB* processPCB, FILE* outLogFile);
void handler(int signum);
bool succesful_exit_handler = false;
void SRTN(FILE* outLogFile);
void HPF(FILE* outLogFile);

int main(int argc, char * argv[])
{
    // Establish communication with the clock module
    initClk();
    
    // Open an output file for the scheduler log (in the write mode)
    FILE* outLogFile = (FILE *) malloc(sizeof(FILE));
    outLogFile = fopen("SchedulerLog.txt", "w");
    if (outLogFile == NULL) {
        printf("Could not open output file for scheduler log.\n");
    }



    // Close the output log file
    fclose(outLogFile);
    // Upon termination, release resources of communication with the clock module
    destroyClk(true);
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
}

void hanlder(int signum) {
    
    int pid, stat_loc;
    printf("\nfrom handler my Id: %d\n",getpid() ); 

    printf("Child has sent a SIGCHLD signal #%d\n",signum);

    pid = wait(&stat_loc);
    if(WIFEXITED(stat_loc))
        succesful_exit_handler = true;
    
}


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
            push(PQueueHead, tempPCB, tempPCB->remainingTime);

            while(receiveMsg(0, tempBuffer)) {
                tempPCB = tempBuffer->data;
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
    } 
}

void HPF(FILE* outLogFile){
    PNode* ReadyQueue = NULL;
    struct msgbuff* message;
    PCB* temp_process_pcb = NULL;
    PCB* ready_process_pcb = NULL;
    
    while (1)
    {
        while (receiveMsg(0,message))
        {
            temp_process_pcb = &(message->data);
            push(&ReadyQueue, temp_process_pcb, temp_process_pcb->priority);
        }
        if (!isEmpty(&ReadyQueue))
        {
            pop(&ReadyQueue,ready_process_pcb);
            startProcess(&ready_process_pcb, outLogFile);
            sleep(ready_process_pcb->runTime);
            
            while (!succesful_exit_handler)
            {
                stopProcess(&ready_process_pcb, outLogFile,0);
                resumeProcess(&ready_process_pcb, outLogFile,0);
                sleep(ready_process_pcb->remainingTime);
            }
            finishProcess(&ready_process_pcb, outLogFile);
        
        }
        
        
    }
    
}