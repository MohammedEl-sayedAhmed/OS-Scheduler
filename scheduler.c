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
int SRTN(FILE* outLogFile);
int SSSS(FILE* outLogFile);
void HPF(FILE* outLogFile);

int succesful_exit_handler = 0;   //global variable to store the handler result of exit code
int finish_scheduler = 0;         //global variable to store if the scheduler should stop (No other processes)
int exitnow;

int main(int argc, char * argv[])
{

    exitnow = 0;
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
        //HPF(outLogFile);  
    
    //}
    printf("finish sched %d", finish_scheduler);
    initMsgQueue();
    //SRTN(outLogFile);
    SSSS(outLogFile);

    //sleep(1000);


    // Close the output log file
    fclose(outLogFile);
    
    // Upon termination, release resources of communication with the clock module
    
    //destroyClk(false);

    return 0;
}

void resumeProcess(PCB* processPCB, FILE* outLogFile, bool silent) {

    // Send a continue signal to the process
    kill(processPCB->pid, SIGCONT);
    printf("Inside resume id %d pid %d\n", processPCB->id, processPCB-> pid);

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
        printf("Will execv\n");
        char str[100];
        sprintf(str, "%d", processPCB->runTime);
        //char *argv[] = { "./process.out", str};
        //execve(argv[0], &argv[0], NULL);
        char * param[] = {str, NULL};
        execv("./process.out", param); // argv is the list of arguments to pass (scheduling algorithm + necessary parameters)

    }
    else {
        int currTime = getClk();

        // Update the process PCB fields as appropriate
        processPCB->pid = pid;
        processPCB->startTime = currTime;
        processPCB->remainingTime = processPCB->runTime;
        processPCB->waitingTime = currTime - processPCB->arrivalTime;
        //printf("Process created successfully.\n");

        // Print the "starting" line in the output log file
        fprintf(outLogFile, "At time %d process %d started arr %d total %d remain %d wait %d\n", currTime, processPCB->id, processPCB->arrivalTime, processPCB->runTime, processPCB->remainingTime, processPCB->waitingTime);

    }
}

void stopProcess(PCB* processPCB, FILE* outLogFile, bool silent) {
    //send a stop signal to the process
    kill(processPCB->pid, SIGSTOP);

    printf("Inside stop id %d pid %d\n", processPCB->id, processPCB-> pid);
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
    processPCB->remainingTime = 0;
    processPCB->finishTime = processPCB->arrivalTime + processPCB->waitingTime + processPCB->runTime;
    fprintf(outLogFile, "At time %d process %d finished arr %d total %d remain %d wait %d\n", processPCB->finishTime, processPCB->id, processPCB->arrivalTime, processPCB->runTime, processPCB->remainingTime, processPCB->waitingTime);
    processPCB = NULL;
    succesful_exit_handler = 0;
}

void handler(int signum) {
    
    int pid, stat_loc;
    printf("\nfrom handler my Id: %d\n",getpid() ); 

    pid = wait(&stat_loc);
    if(WIFEXITED(stat_loc))
    {
        printf("Child with pid %d has sent a SIGCHLD signal #%d\n", pid, signum);
        succesful_exit_handler = 1;
    }
    else
    {
        succesful_exit_handler = 0;
        printf("Child with pid %d has sent a SIGCHLD signal #%d\n", pid, signum);
    }
}


int SRTN(FILE* outLogFile) {

    printf("Inside SRTN\n");
    PNode* PQueueHead = NULL;
    struct msgbuff tempBuffer;
    PCB tempPCB;
    int status;
    //bool silence = false; //////////

    while (exitnow!=1) {  ////te2felllll + silent in file or not

        printf("Inisde while 1");
        //if ((isEmpty(&PQueueHead)) || (!isEmpty(&PQueueHead))) {
        if (isEmpty(&PQueueHead)) {

            if(finish_scheduler) {
                printf("will break right -10 is empty%d\n", isEmpty(&PQueueHead));
                exitnow = 1;
                continue;
            }

            tempBuffer = receiveMsg(1, &status);
            printf("Status after rec %d\n", status);
            //if(status) {
            //    printf("pcb pid bef %d", tempBuffer.data.arrivalTime);
            //}

            if(status) {
                equate(&tempBuffer.data, &tempPCB);    
                //tempPCB = tempBuffer.data;
                printf("pcb pid %d", tempPCB.pid);
                if (tempPCB.pid == -10)
                {
                    printf("will break -10\n");
                    exitnow = 1;
                    continue;
                }
                else {
                push(&PQueueHead, &tempPCB, tempPCB.remainingTime);
                printf("pushed id %d is empty %d\n", tempPCB.id, isEmpty(&PQueueHead));
                }
            }
            /*
            tempBuffer = receiveMsg(0, &status);
            while(status) {
                equate(&tempBuffer.data, &tempPCB); 
                printf("pcb pid %d", tempPCB.pid);   
            //    tempPCB = tempBuffer.data;
                if (tempPCB.pid == -10)
                {
                    finish_scheduler = 1;
                    //break;
                }
                push(&PQueueHead, &tempPCB, tempPCB.remainingTime);
                tempBuffer = receiveMsg(0, &status);
            }
            */
        }

        PCB* currProcessPCB = (PCB *) malloc(sizeof(PCB));      
        pop(&PQueueHead, currProcessPCB);
        
        //printf("PIDDDDDDDDDDDD %d\n", currProcessPCB->pid);
        if (currProcessPCB->pid == -5) {
            startProcess(currProcessPCB, outLogFile);
            printf("STARTTTT id %d pid%d\n", currProcessPCB->id, currProcessPCB->pid);
        }
        else {
            resumeProcess(currProcessPCB, outLogFile, 0);
        }
        
        if(!finish_scheduler) {
            tempBuffer = receiveMsg(1, &status);
            printf("Status after rec %d\n", status);
        }
        else {
            sleep(currProcessPCB->remainingTime);
        }

        if (succesful_exit_handler) {
            printf("bef finish process\n");
            finishProcess(currProcessPCB, outLogFile); /////////////////////// set successful exit handler
        }
        else {
            if (tempBuffer.data.pid != -10) {
            stopProcess(currProcessPCB, outLogFile, 0);
            equate(&tempBuffer.data, &tempPCB);    
            //tempPCB = tempBuffer.data;
            push(&PQueueHead, &tempPCB, tempPCB.remainingTime);
            push(&PQueueHead, currProcessPCB, currProcessPCB->remainingTime);
            printf("pushed id %d is empty %d\n", tempPCB.id, isEmpty(&PQueueHead));
            }
            else {
                finish_scheduler = 1;
            }

        }
        /*
        tempBuffer = receiveMsg(0, &status);
        while(status) {
            equate(&tempBuffer.data, &tempPCB);    
        //    tempPCB = tempBuffer.data;
            printf("pcb pid %d", tempPCB.pid);
            push(&PQueueHead, &tempPCB, tempPCB.remainingTime);
            tempBuffer = receiveMsg(0, &status);
        }
        */
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
    printf("Outside while\n"); 
    return 1;
}

int SSSS(FILE* outLogFile) {

    printf("Inside SSSS\n");
    PNode* PQueueHead = NULL;
    struct msgbuff tempBuffer;
    PCB tempPCB;
    int status;

    while(1) {
        printf("Inside While\n");
        if (isEmpty(&PQueueHead)) {

            if(finish_scheduler) {
                printf("Empty %d and finish is true\n", isEmpty(&PQueueHead));
                break;
            }

            printf("Will wait for msg up");
            tempBuffer = receiveMsg(1, &status);

            if(status) {
                equate(&tempBuffer.data, &tempPCB);    
                printf("Received id %d pid %d", tempPCB.id, tempPCB.pid);
                if (tempPCB.pid == -10)
                {
                    printf("Empty %d and received -10\n", isEmpty(&PQueueHead));
                    break;
                }
                else {
                    push(&PQueueHead, &tempPCB, tempPCB.remainingTime);
                    printf("pushed id %d pid %d is empty %d\n", tempPCB.id, tempPCB.pid,isEmpty(&PQueueHead));
                }
            }
            else {
                printf("Empty and error in receive\n");
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
        
        if(!finish_scheduler) {
            printf("Will wait for msg down");
            tempBuffer = receiveMsg(1, &status);
        }
        else {
            sleep(currProcessPCB->remainingTime); ////////////////
        }

        if (succesful_exit_handler) {
            printf("Before finish process id %d\n", currProcessPCB->id);
            finishProcess(currProcessPCB, outLogFile); /////////////////////// set successful exit handler
            printf("Finished process id %d\n", currProcessPCB->id);
        }
        else if (status == 1) {
            if (tempBuffer.data.pid != -10) {
                stopProcess(currProcessPCB, outLogFile, 0);
                equate(&tempBuffer.data, &tempPCB);    
                push(&PQueueHead, &tempPCB, tempPCB.remainingTime);
                push(&PQueueHead, currProcessPCB, currProcessPCB->remainingTime);
                printf("Pushed id %d pid %d is empty %d\n", tempPCB.id, tempPCB.pid, isEmpty(&PQueueHead));
            }
            else {
                stopProcess(currProcessPCB, outLogFile, 0);
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
    return 0;
}

/*
void HPF(FILE* outLogFile){
    1
    PNode* ReadyQueue = NULL;
    struct msgbuff* message;
    PCB* temp_process_pcb = NULL;
    PCB* ready_process_pcb = NULL;
    //printf("finish scheduler %d   ", finish_scheduler);
    while (1)
    {
        //printf("finish scheduler %d   ", finish_scheduler);
        if (isEmpty(&ReadyQueue))
        {
            //printf("finish scheduler %d   ", finish_scheduler);
            //if (finish_scheduler == true)
            //{
            //    printf("HPPPPPPPPPFFFFFFF");
            //    break;
            //}
            receiveMsg(1,message);
            temp_process_pcb = &(message->data);
            //if (temp_process_pcb->pid == -10)
            //{
            //    printf("true1/n");
            //    finish_scheduler = true;
            //    break;
            //}
            push(&ReadyQueue, temp_process_pcb, temp_process_pcb->priority);
        }
        
        while (receiveMsg(0,message))
        {
            temp_process_pcb = &(message->data);
            //if (temp_process_pcb->pid == -10)
            //{
            //    printf("true1/n");
            //    finish_scheduler = true;
            //}
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
*/