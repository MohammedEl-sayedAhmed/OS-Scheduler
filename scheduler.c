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


void userHandler(int signum);
void startProcess(PCB* processPCB, FILE* outLogFile);
void resumeProcess(PCB* processPCB, FILE* outLogFile, bool silent);
void stopProcess(PCB* processPCB, FILE* outLogFile, bool silent);
void finishProcess(PCB* processPCB, FILE* outLogFile);
void SRTN(FILE* outLogFile);
void HPF(FILE* outLogFile);
void RR(FILE* outLogFile, int Quantum);
int succesful_exit_handler = 0;   // global variable to store that the child exited successfully
int finish_scheduler = 0;         // global variable to store if the scheduler should stop (No other processes)

int total_waiting_time = 0;
int total_proceesing_time = 0;
int total_WTA = 0;
int processCount = 0;

int main(int argc, char * argv[])
{

    // Establish communication with the clock module
    initClk();

    // Handler for SIGUSR1 signal sent by children to the scheduler upon successful termination
    signal(SIGUSR1, userHandler);
    
  
    // Open an output file for the scheduler log (in the write mode)
    FILE* outLogFile = (FILE *) malloc(sizeof(FILE));
    outLogFile = fopen("SchedulerLog.txt", "w");
    if (outLogFile == NULL) {
        printf("Could not open output file for scheduler log.\n");
    }

    // Open an output file for the scheduler calculations (in the write mode)
    FILE* outCalcFile = (FILE *) malloc(sizeof(FILE));
    outCalcFile = fopen("SchedulerCalc.txt", "w");
    if (outCalcFile == NULL) {
        printf("Could not open output file for scheduler calculations.\n");
    }

    // Read the passed arguments 
    int quantum;
    char *schedalg = NULL;

    if (argc == 2) {
        // Get the chosen algorithm 
        schedalg = argv[0];
        ///////printf("Algorithm chosen: %s.\n", schedalg); 

        // Get the quantum needed for round robin algorithm 
        quantum = atoi(argv[1]);
        ///////printf("Quantum chosen: %d.\n", quantum);
    }

    // Initialize message queue 
    initMsgQueue();

    // Run the chosen algorithm
    if(strcmp(schedalg,"HPF") == 0)
    {
        printf("Chosen algorithm is HPF.\n");
        HPF(outLogFile);  
    }
    else if (strcmp(schedalg,"RR") == 0) {
        printf("Chosen algorithm is RR with a quantum of %d seconds.\n", quantum);
        RR(outLogFile, quantum);
    }
    else if (strcmp(schedalg,"SRTN") == 0) {
        printf("Chosen algorithm is SRTN.\n");
        SRTN(outLogFile);
    }

    // Close the output log and calculations file
    fclose(outLogFile);
    fclose(outCalcFile);
    
    // Upon termination, release resources of communication with the clock module
    //destroyClk(false);

    return 0;
}

void resumeProcess(PCB* processPCB, FILE* outLogFile, bool silent) {

    // Send a continue signal to the process
    kill(processPCB->pid, SIGCONT);
    printf("Inside resume id %d pid %d\n", processPCB->id, processPCB->pid);

    // Calculate and update the process waiting time and state
    int currTime = getClk();
    processPCB->waitingTime = (currTime - processPCB->arrivalTime) - (processPCB->runTime - processPCB->remainingTime);
    processPCB->state = RUNNING;

    // Print the "resumed" line in the output log file
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
        printf("Will execv\n");
        char str[100];
        sprintf(str, "%d", processPCB->runTime);
        char * param[] = {str, NULL};
        execv("./process.out", param); // param contains the running time of the process

    }
    else {
        int currTime = getClk();

        // Update the process PCB fields as appropriate
        processPCB->pid = pid;
        processPCB->startTime = currTime;
        processPCB->remainingTime = processPCB->runTime;
        processPCB->waitingTime = currTime - processPCB->arrivalTime;
        processPCB->state = RUNNING;
        //printf("Process created successfully.\n");

        // Print the "started" line in the output log file
        fprintf(outLogFile, "At time %d process %d started arr %d total %d remain %d wait %d\n", currTime, processPCB->id, processPCB->arrivalTime, processPCB->runTime, processPCB->remainingTime, processPCB->waitingTime);
    }
}

void stopProcess(PCB* processPCB, FILE* outLogFile, bool silent) {

    // Send a stop signal to the process
    kill(processPCB->pid, SIGSTOP);

    printf("Inside stop id %d pid %d\n", processPCB->id, processPCB-> pid);
    // Calculate and update the process remaining time and state
    int currTime = getClk();
    processPCB->remainingTime = (processPCB->runTime) -  (currTime - processPCB->arrivalTime - processPCB->waitingTime);
    processPCB->state = WAITING;

    // Print the "stopped" line in the output log file
    if(!silent) {
        fprintf(outLogFile, "At time %d process %d stopped arr %d total %d remain %d wait %d\n", currTime, processPCB->id, processPCB->arrivalTime, processPCB->runTime, processPCB->remainingTime, processPCB->waitingTime);
    }
}

void finishProcess(PCB* processPCB, FILE* outLogFile)
{
    int currTime = getClk();

    // Calculate and update the process remaining time, finish time and state
    processPCB->remainingTime = 0;
    processPCB->finishTime = processPCB->arrivalTime + processPCB->waitingTime + processPCB->runTime;
    processPCB->state = FINISHED;
    fprintf(outLogFile, "At time %d process %d finished arr %d total %d remain %d wait %d\n", currTime, processPCB->id, processPCB->arrivalTime, processPCB->runTime, processPCB->remainingTime, processPCB->waitingTime);
    free(processPCB);
    processPCB = NULL;
    succesful_exit_handler = 0;
}

void userHandler(int signum) {
    printf("Process successfully terminated.\n");
    succesful_exit_handler = 1;
}

// Shortest Remaining Time Next Algorithm
void SRTN(FILE* outLogFile) {

    printf("Running SRTN\n.");
    PNode* PQueueHead = NULL;
    struct msgbuff tempBuffer;
    PCB* tempPCB = NULL;
    int status;

    while(1) {
        //printf("Inside While\n");
        if (isEmpty(&PQueueHead)) {

            if(finish_scheduler) {
                printf("Empty %d and finish is true\n", isEmpty(&PQueueHead));
                break;
            }

            status = 0;
            while(!status) {
                printf("Will wait for msg up\n");
                tempBuffer = receiveMsg(1, &status);
            }

            //printf("Will wait for msg up");
            //tempBuffer = receiveMsg(1, &status);

            if(status) {
                //equate(&tempBuffer.data, &tempPCB);    
                printf("Received id %d pid %d\n", tempBuffer.data.id, tempBuffer.data.pid);
                if (tempBuffer.data.pid == -10)
                {
                    printf("Empty %d and received -10\n", isEmpty(&PQueueHead));
                    break;
                }
                else {
                    tempPCB = (PCB *) malloc(sizeof(PCB));  
                    equate(&tempBuffer.data, tempPCB); 
                    push(&PQueueHead, tempPCB, tempPCB->remainingTime);
                    printf("pushed id %d pid %d is empty %d\n", tempPCB->id, tempPCB->pid,isEmpty(&PQueueHead));
                }
            }
            else {
                printf("Empty and error in receive\n");
            }
/*
            status = 0;
            tempBuffer = receiveMsg(0, &status);
            while(status) {
                equate(&tempBuffer.data, &tempPCB); 
                printf("pcb pid %d", tempPCB.pid);   
            //    tempPCB = tempBuffer.data;
                if (tempPCB.pid == -10)
                {
                    finish_scheduler = 1;
                    status = 0;
                    //break;
                }
                else {
                    push(&PQueueHead, &tempPCB, tempPCB.remainingTime);
                    status = 0;
                    tempBuffer = receiveMsg(0, &status);
                }
            }
*/            
        }

        status = 0;
        tempBuffer = receiveMsg(0, &status);
        while(status == 1) {
            printf("pcb pid %d\n", tempBuffer.data.pid);   
        //    tempPCB = tempBuffer.data;
            if (tempBuffer.data.pid == -10)
            {
                finish_scheduler = 1;
                status = 0;
                //break;
            }
            else {
                tempPCB = (PCB *) malloc(sizeof(PCB));  
                equate(&tempBuffer.data, tempPCB); 
                push(&PQueueHead, tempPCB, tempPCB->remainingTime);
                status = 0;
                tempBuffer = receiveMsg(0, &status);
            }
        }

        PCB* currProcessPCB = (PCB *) malloc(sizeof(PCB));      
        int afterPop = pop(&PQueueHead, currProcessPCB);
        
        printf("Success pop %d Popped id %d pid %d\n", afterPop, currProcessPCB->id, currProcessPCB->pid);
        if (currProcessPCB->pid == -5) {
            startProcess(currProcessPCB, outLogFile);
            printf("Started id %d pid %d\n", currProcessPCB->id, currProcessPCB->pid);
        }
        else {
            resumeProcess(currProcessPCB, outLogFile, 0);
        }

        succesful_exit_handler = 0;        
        if(!finish_scheduler) {
            printf("Will wait for msg down\n"); ////////////7ot while ll rec
            status = 0;
            tempBuffer = receiveMsg(1, &status);
        }
        else {
            int stat;
            pid_t isCurrProccess = waitpid(currProcessPCB->pid, &stat, 0);
            if ((isCurrProccess != currProcessPCB->pid) || !(WIFEXITED(stat))) {
                printf("Signal received from process %d.\n", WEXITSTATUS(stat));
                if (WIFSIGNALED(stat)) {
                    psignal(WTERMSIG(stat), "Exit signal");  
                }       
            }
            printf("Signal received from process %d.\n", WIFEXITED(stat));
            printf("After sleep\n");
        }

        if (succesful_exit_handler) {
            printf("Before finish process id %d\n", currProcessPCB->id);
            finishProcess(currProcessPCB, outLogFile); /////////////////////// set successful exit handler
            printf("Finished process id %d\n", currProcessPCB->id);
        }
        else if (status == 1) {
            if (tempBuffer.data.pid != -10) {
                stopProcess(currProcessPCB, outLogFile, 0);
                succesful_exit_handler = 0;
                tempPCB = (PCB *) malloc(sizeof(PCB));  
                equate(&tempBuffer.data, tempPCB);    
                push(&PQueueHead, currProcessPCB, currProcessPCB->remainingTime);
                push(&PQueueHead, tempPCB, tempPCB->remainingTime);
                printf("Pushed id %d pid %d is empty %d\n", tempPCB->id, tempPCB->pid, isEmpty(&PQueueHead));
            }
            else {
                stopProcess(currProcessPCB, outLogFile, 0);
                succesful_exit_handler = 0;
                resumeProcess(currProcessPCB, outLogFile, 0);
                printf("Will sleep to finish process pid %d\n", currProcessPCB->pid);
                //sleep(currProcessPCB->remainingTime);
                //sleep(10);
                int stat;
                pid_t isCurrProccess = waitpid(currProcessPCB->pid, &stat, 0);
                while ((isCurrProccess != currProcessPCB->pid) || !(WIFEXITED(stat))) {
                    printf("Signal received from process %d.\n", WEXITSTATUS(stat));
                    if (WIFSIGNALED(stat)) {
                        psignal(WTERMSIG(stat), "Exit signal");  
                    }       
                }
                printf("Signal received from process %d.\n", WIFEXITED(stat));
                printf("After sleep\n");
                finishProcess(currProcessPCB, outLogFile); /////////////////////// set successful exit handler
                printf("Finished process id %d\n", currProcessPCB->id);
                finish_scheduler = 1;
            }
        }
    }
    printf("Outside While\n");
    return;
}

void RR(FILE* outLogFile, int Quantum) {

    printf("Inside RRRR\n");
    Queue readyQueue;
    queueInit(&readyQueue, sizeof(PCB));
    struct msgbuff tempBuffer;
    PCB* tempPCB = NULL;
    int status;
    finish_scheduler = 0;

    while(1) {
        printf("Inside While\n");
        if (getQueueSize(&readyQueue) == 0) {

            //printf("Breaking\n");
            //break;

            if(finish_scheduler) {
                printf("Empty %d and finish is true\n", getQueueSize(&readyQueue));
                break;
            }

            status = 0;
            while(!status) {
                printf("Will wait for msg up\n");
                tempBuffer = receiveMsg(1, &status);
            }

            if(status) {
                //equate(&tempBuffer.data, &tempPCB);    
                printf("Received id %d pid %d\n", tempBuffer.data.id, tempBuffer.data.pid);
                if (tempBuffer.data.pid == -10) {
                    printf("Empty %d and received -10\n", getQueueSize(&readyQueue));
                    break;
                }
                else {
                    tempPCB = (PCB *) malloc(sizeof(PCB));  
                    equate(&tempBuffer.data, tempPCB);
                    enqueue(&readyQueue, tempPCB);
                    printf("Enqueued id %d pid %d is empty %d\n", tempPCB->id, tempPCB->pid, getQueueSize(&readyQueue));
                }
            }
            else {
                printf("Empty and error in receive\n");
            }
        }

        status = 0;
        tempBuffer = receiveMsg(0, &status);
        while(status == 1) {
            printf("PCB pid %d\n", tempBuffer.data.pid);   
        //    tempPCB = tempBuffer.data;
            if (tempBuffer.data.pid == -10)
            {
                finish_scheduler = 1;
                status = 0;
                //break;
            }
            else {
                tempPCB = (PCB *) malloc(sizeof(PCB));  
                equate(&tempBuffer.data, tempPCB); 
                enqueue(&readyQueue, tempPCB);
                printf("Enqueued process id %d", tempPCB->id);
                status = 0;
                tempBuffer = receiveMsg(0, &status);
            }
        }

        PCB* currProcessPCB = (PCB *) malloc(sizeof(PCB));      
        int afterDeq = dequeue(&readyQueue, currProcessPCB);
        
        printf("Success dequeue %d Dequeued id %d pid %d\n", afterDeq, currProcessPCB->id, currProcessPCB->pid);
        if (currProcessPCB->pid == -5) {
            startProcess(currProcessPCB, outLogFile);
            printf("Started id %d pid %d\n", currProcessPCB->id, currProcessPCB->pid);
        }
        else {
            resumeProcess(currProcessPCB, outLogFile, 0);
        }
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
        if (currProcessPCB->remainingTime <= Quantum) {

            int stat;
            pid_t isCurrProccess = waitpid(currProcessPCB->pid, &stat, 0);
            if ((isCurrProccess != currProcessPCB->pid) || !(WIFEXITED(stat))) {
                printf("Signal received from process %d.\n", WEXITSTATUS(stat));
                if (WIFSIGNALED(stat)) {
                    psignal(WTERMSIG(stat), "Exit signal");  
                }       
            }
            printf("Signal received from process, exit code %d.\n", WIFEXITED(stat));
            printf("After sleep\n");

            // check for handler 
///////////////////////////////////////////////////////////////////////////////////////////////////////////
            while (!succesful_exit_handler)
            {
                stopProcess(currProcessPCB, outLogFile, 1);
                resumeProcess(currProcessPCB, outLogFile, 1);
                int stat;
                pid_t isCurrProccess = waitpid(currProcessPCB->pid, &stat, 0);
                if ((isCurrProccess != currProcessPCB->pid) || !(WIFEXITED(stat))) {
                    printf("Signal received from process %d.\n", WEXITSTATUS(stat));
                    if (WIFSIGNALED(stat)) {
                        psignal(WTERMSIG(stat), "Exit signal");  
                    }       
                }
                printf("Signal received from process, exit code %d.\n", WIFEXITED(stat));
                printf("After sleep\n");
            }
///////////////////////////////////////////////////////////////////////////////////////////////////////////
            finishProcess(currProcessPCB, outLogFile);
/*
            status = 0;
            tempBuffer = receiveMsg(0, &status);
            while(status == 1) {
                printf("PCB pid %d\n", tempBuffer.data.pid);   
            //    tempPCB = tempBuffer.data;
                if (tempBuffer.data.pid == -10)
                {
                    finish_scheduler = 1;
                    status = 0;
                    //break;
                }
                else {
                    tempPCB = (PCB *) malloc(sizeof(PCB));  
                    equate(&tempBuffer.data, tempPCB); 
                    enqueue(&readyQueue, tempPCB);
                    printf("Enqueued process id %d", tempPCB->id);
                    status = 0;
                    tempBuffer = receiveMsg(0, &status);
                }
            }*/
        }
        else {
            printf("Not sure sleep\n");
            sleep(Quantum);

            printf("Stopping process id %d\n", currProcessPCB->id);
            stopProcess(currProcessPCB, outLogFile, 0);

            status = 0;
            tempBuffer = receiveMsg(0, &status);
            while(status == 1) {
                printf("PCB pid %d\n", tempBuffer.data.pid);   
            //    tempPCB = tempBuffer.data;
                if (tempBuffer.data.pid == -10)
                {
                    finish_scheduler = 1;
                    status = 0;
                    //break;
                }
                else {
                    tempPCB = (PCB *) malloc(sizeof(PCB));  
                    equate(&tempBuffer.data, tempPCB); 
                    enqueue(&readyQueue, tempPCB);
                    printf("Enqueued process id %d", tempPCB->id);
                    status = 0;
                    tempBuffer = receiveMsg(0, &status);
                }
            }
            enqueue(&readyQueue, currProcessPCB);
        }  
    }
    printf("Outside While\n");
    return;
}

void HPF(FILE* outLogFile) {
    printf("Inside HHH\n");
    PNode* ReadyQueue = NULL;
    struct msgbuff tempBuffer;
    PCB* temp_process_pcb = (PCB *) malloc(sizeof(PCB));
    int status;
    finish_scheduler = 0;

    while(1) {
        printf("Inside While\n");
        if (isEmpty(&ReadyQueue)) {

            if(finish_scheduler) {
                printf("Empty %d and finish is true\n", isEmpty(&ReadyQueue));
                break;
            }

            status = 0;
            while(!status) {
                printf("Will wait for msg up\n");
                tempBuffer = receiveMsg(1, &status);
            }

            //printf("Will wait for msg up");
            //tempBuffer = receiveMsg(1, &status);

            if(status) {
                //equate(&tempBuffer.data, &tempPCB);    
                printf("Received id %d pid %d\n", tempBuffer.data.id, tempBuffer.data.pid);
                if (tempBuffer.data.pid == -10)
                {
                    printf("Empty %d and received -10\n", isEmpty(&ReadyQueue));
                    break;
                }
                else {
                    temp_process_pcb = (PCB *) malloc(sizeof(PCB)); 
                    equate(&tempBuffer.data, temp_process_pcb); 
                    push(&ReadyQueue, temp_process_pcb, temp_process_pcb->priority);
                    printf("pushed id %d pid %d is empty %d\n", temp_process_pcb->id, temp_process_pcb->pid,isEmpty(&ReadyQueue));
                }
            }
            else {
                printf("Empty and error in receive\n");
            }
        }

        status = 0;
        tempBuffer = receiveMsg(0, &status);
        while(status == 1) {
            printf("pcb pid %d\n", tempBuffer.data.pid);   
        //    tempPCB = tempBuffer.data;
            if (tempBuffer.data.pid == -10)
            {
                finish_scheduler = 1;
                status = 0;
                //break;
            }
            else {
                temp_process_pcb = (PCB *) malloc(sizeof(PCB)); 
                equate(&tempBuffer.data, temp_process_pcb); 
                push(&ReadyQueue, temp_process_pcb, temp_process_pcb->priority);
                printf("pushed id %d pid %d is empty %d\n", temp_process_pcb->id, temp_process_pcb->pid,isEmpty(&ReadyQueue));
                status = 0;
                tempBuffer = receiveMsg(0, &status);
            }
        }

        PCB* currProcessPCB = (PCB *) malloc(sizeof(PCB));      
        int afterPop = pop(&ReadyQueue, currProcessPCB);
        
        printf("Success pop %d Popped id %d pid %d\n", afterPop, currProcessPCB->id, currProcessPCB->pid);
        if (currProcessPCB->pid == -5) {
            startProcess(currProcessPCB, outLogFile);
            printf("Started id %d pid %d\n", currProcessPCB->id, currProcessPCB->pid);
        }
        //else {
        //    resumeProcess(currProcessPCB, outLogFile, 0);
        //}
        int stat;
        pid_t isCurrProccess = waitpid(currProcessPCB->pid, &stat, 0);
        if ((isCurrProccess != currProcessPCB->pid) || !(WIFEXITED(stat))) {
            printf("Signal received from process %d.\n", WEXITSTATUS(stat));
            if (WIFSIGNALED(stat)) {
                psignal(WTERMSIG(stat), "Exit signal");  
            }       
        }
        printf("Signal received from process, exit code %d.\n", WIFEXITED(stat));
        printf("After sleep\n");

        while (!succesful_exit_handler)
        {
            stopProcess(currProcessPCB, outLogFile, 1);
            resumeProcess(currProcessPCB, outLogFile, 1);
            int stat;
            pid_t isCurrProccess = waitpid(currProcessPCB->pid, &stat, 0);
            if ((isCurrProccess != currProcessPCB->pid) || !(WIFEXITED(stat))) {
                printf("Signal received from process %d.\n", WEXITSTATUS(stat));
                if (WIFSIGNALED(stat)) {
                    psignal(WTERMSIG(stat), "Exit signal");  
                }       
            }
            printf("Signal received from process, exit code %d.\n", WIFEXITED(stat));
            printf("After sleep\n");
        }
        finishProcess(currProcessPCB, outLogFile);
        /*
        status = 0;
        tempBuffer = receiveMsg(0, &status);
        while(status == 1) {
            printf("pcb pid %d\n", tempBuffer.data.pid);   
        //    tempPCB = tempBuffer.data;
            if (tempBuffer.data.pid == -10)
            {
                finish_scheduler = 1;
                status = 0;
                //break;
            }
            else {
                temp_process_pcb = (PCB *) malloc(sizeof(PCB)); 
                equate(&tempBuffer.data, temp_process_pcb); 
                push(&ReadyQueue, temp_process_pcb, temp_process_pcb->priority);
                printf("After push\n");
                status = 0;
                tempBuffer = receiveMsg(0, &status);
            }
        }
        */
    }
    return;
}