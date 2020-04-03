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
void userHandler(int signum);
int SRTN(FILE* outLogFile);
int HPF(FILE* outLogFile);
struct msgbuff message_hpf;
void RR(FILE* outLogFile, int Quantum);
void RRRR(FILE* outLogFile, int Quantum);


int succesful_exit_handler = 0;   //global variable to store the handler result of exit code
int finish_scheduler = 0;         //global variable to store if the scheduler should stop (No other processes)
int exitnow;

int main(int argc, char * argv[])
{

    exitnow = 0;
    // Establish communication with the clock module
    initClk();

    // Calling the written handler when the child exits 
    //signal(SIGCHLD, handler);
    signal(SIGUSR1, userHandler);
    
  
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
//    SRTN(outLogFile);

//    HPF(outLogFile);

    RRRR(outLogFile, 8);

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
        printf("Inside start id %d pid %d", processPCB->id, processPCB->pid);

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
    fprintf(outLogFile, "At time %d process %d finished arr %d total %d remain %d wait %d\n", currTime, processPCB->id, processPCB->arrivalTime, processPCB->runTime, processPCB->remainingTime, processPCB->waitingTime);
    processPCB = NULL;
    succesful_exit_handler = 0;
}

/*
void handler(int signum) {
    
    int pid, stat_loc;
    printf("\nfrom handler my Id: %d\n",getpid() ); 

    succesful_exit_handler = 1;
    

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
*/

void userHandler(int signum) {
    printf("User Signal\n");
    succesful_exit_handler = 1;
}

int SRTN(FILE* outLogFile) {

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
                    equate(&tempBuffer.data, &tempPCB); 
                    push(&PQueueHead, &tempPCB, tempPCB.remainingTime);
                    printf("pushed id %d pid %d is empty %d\n", tempPCB.id, tempPCB.pid,isEmpty(&PQueueHead));
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
                equate(&tempBuffer.data, &tempPCB); 
                push(&PQueueHead, &tempPCB, tempPCB.remainingTime);
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
                equate(&tempBuffer.data, &tempPCB);    
                push(&PQueueHead, currProcessPCB, currProcessPCB->remainingTime);
                push(&PQueueHead, &tempPCB, tempPCB.remainingTime);
                printf("Pushed id %d pid %d is empty %d\n", tempPCB.id, tempPCB.pid, isEmpty(&PQueueHead));
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
    return 0;
}

void RRRR(FILE* outLogFile, int Quantum) {

    printf("Inside RRRR\n");
    Queue readyQueue;
    queueInit(&readyQueue, sizeof(PCB));
    struct msgbuff tempBuffer;
    PCB tempPCB;
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
                    equate(&tempBuffer.data, &tempPCB);
                    enqueue(&readyQueue, &tempPCB);
                    printf("Enqueued id %d pid %d is empty %d\n", tempPCB.id, tempPCB.pid, getQueueSize(&readyQueue));
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
                equate(&tempBuffer.data, &tempPCB); 
                enqueue(&readyQueue, &tempPCB);
                printf("Enqueued process id %d", tempPCB.id);
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
        if (currProcessPCB->remainingTime < Quantum) {

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
                    equate(&tempBuffer.data, &tempPCB); 
                    enqueue(&readyQueue, &tempPCB);
                    printf("Enqueued process id %d", tempPCB.id);
                    status = 0;
                    tempBuffer = receiveMsg(0, &status);
                }
            }
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
                    equate(&tempBuffer.data, &tempPCB); 
                    enqueue(&readyQueue, &tempPCB);
                    printf("Enqueued process id %d", tempPCB.id);
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



int HPF(FILE* outLogFile){
    PNode* ReadyQueue = NULL;
    PCB* temp_process_pcb = NULL;
    PCB* ready_process_pcb = NULL;

    while (1)
    {
        if (isEmpty(&ReadyQueue))
        {
            if (finish_scheduler == 1)
            {
                printf("finished scheduler \n");
                break;
            }
            int rec_val = msgrcv(msgqid, &message_hpf, sizeof(message_hpf), 0, !IPC_NOWAIT);
            printf("msg rec %d   \n", rec_val);
            temp_process_pcb = &(message_hpf.data);
            if (temp_process_pcb->pid == -10)
            {
                printf("true1 /n");
                finish_scheduler = 1;
                break;
            }
            push(&ReadyQueue, temp_process_pcb, temp_process_pcb->priority);
    }

        while (1)
        {
            int rec_val = msgrcv(msgqid, &message_hpf, sizeof(message_hpf), 0, IPC_NOWAIT);
            printf("2- msg rec %d   \n", rec_val);
            if (rec_val == -1)
            {
                break;
            }
            temp_process_pcb = &(message_hpf.data);
            if (temp_process_pcb->pid == -10)
            {
                printf("true1/n");
                finish_scheduler = 1;
            }
            push(&ReadyQueue, temp_process_pcb, temp_process_pcb->priority);
        }
        PCB* ready_process_pcb = (PCB *) malloc(sizeof(PCB));      

        if (!isEmpty(&ReadyQueue))
        {
            printf("excuteee /n");
            int afterPop = pop(&ReadyQueue, ready_process_pcb);
            printf("afterPop is %d \n", afterPop);
            //pop(&ReadyQueue,ready_process_pcb);
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
    
    


    return 0;
    
}





void RR(FILE* outLogFile, int Quantum) {


    Queue RRreadyQueue;
    queueInit(&RRreadyQueue, sizeof(PCB));
    int size = getQueueSize(&RRreadyQueue);
    struct msgbuff message;
    PCB* temp_process_pcb = NULL;
    PCB* ready_process_pcb = NULL;
    int status;

    
    //printf("finish scheduler %d   ", finish_scheduler);
    while (1)
    {
        //printf("finish scheduler %d   ", finish_scheduler);
        if (getQueueSize(&RRreadyQueue) == 0)
        {
            //printf("finish scheduler %d   ", finish_scheduler);
            /*if (finish_scheduler == true)
            {
                printf("HPPPPPPPPPFFFFFFF");
                break;
            }*/

            message = receiveMsg(1, &status);
            temp_process_pcb = &(message.data);
            /*if (temp_process_pcb->pid == -10)
            {
                printf("true1/n");
                finish_scheduler = true;
                break;
            }*/
            enqueue(&RRreadyQueue, temp_process_pcb);

        }

        status = 0;
        message = receiveMsg(0, &status);
        while (status)
        {
            temp_process_pcb = &(message.data);
            /*if (temp_process_pcb->pid == -10)
            {
                printf("true1/n");
                finish_scheduler = true;
            }*/
            enqueue(&RRreadyQueue, temp_process_pcb);
            status = 0;
            message = receiveMsg(0, &status);
        }

        if (getQueueSize(&RRreadyQueue) != 0)
        {
            dequeue(&RRreadyQueue, ready_process_pcb);
            if (ready_process_pcb->pid == -5) {
                startProcess(ready_process_pcb, outLogFile);
            }
            else {
                resumeProcess(ready_process_pcb, outLogFile,0);

            }
             /////// hna 3ayz a3ml resume aw start 3la 7asb hya awl mra wla la2 

            if (ready_process_pcb->runTime < Quantum) {

                sleep(ready_process_pcb->runTime);

                // check for handler 
///////////////////////////////////////////////////////////////////////////////////////////////////////////
                while (!succesful_exit_handler)
                {
                    stopProcess(ready_process_pcb, outLogFile, 1);
                    resumeProcess(ready_process_pcb, outLogFile, 1);
                    sleep(ready_process_pcb->remainingTime);
                }
///////////////////////////////////////////////////////////////////////////////////////////////////////////

                finishProcess(ready_process_pcb, outLogFile);
            }
            else {
                sleep(Quantum);

                // check for handler 
///////////////////////////////////////////////////////////////////////////////////////////////////////////
                while (!succesful_exit_handler)
                {
                    stopProcess(ready_process_pcb, outLogFile, 1);
                    resumeProcess(ready_process_pcb, outLogFile, 1);
                    sleep(ready_process_pcb->remainingTime);
                }
///////////////////////////////////////////////////////////////////////////////////////////////////////////

                enqueue(&RRreadyQueue, ready_process_pcb);
            }  
        }
    }
}