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
#include <time.h>
#include <math.h>


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

// global variables initialized for calculations
double total_waiting_time = 0;
double total_proceesing_time = 0;
double total_WTA = 0;
double processCount = 0;
Queue WTAQueue;

void StandardDeviation(FILE* outCalcFile);

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

    // Initialize queue of weighted turnaround times
    queueInit(&WTAQueue, sizeof(double));

    // Get initial value of CPU clocks used by scheduler
    clock_t befClocks = clock();
    ///////printf("Initial value of scheduler CPU clocks %ld\n", befClocks);

    // Run the chosen algorithm (HPF, RR or SRTN)
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

    // Get final value of CPU clocks used by scheduler
    clock_t aftClocks = clock();
    ///////printf("Final value of scheduler CPU clocks %ld\n", aftClocks);

    // Total CPU clocks used by scheduler
    clock_t totalClks = aftClocks - befClocks;

    // Calculate total time taken by scheduler in seconds
    double totalSched = ((double) totalClks) + (((double) total_proceesing_time) * ((double) CLOCKS_PER_SEC));
    totalSched = (double) (totalSched/CLOCKS_PER_SEC);
    
    // Calculate CPU utilization and print it in output file
    double cpu_utilization= (double) ((total_proceesing_time/totalSched)*100);
    fprintf(outCalcFile, "CPU  utilization: %.0f %% \n", cpu_utilization);
    
    // Calculate average weighted turnaround time and print it in output file
    double AWTA = (double) (total_WTA/processCount);
    fprintf(outCalcFile,"Avg WTA = %.2f\n", AWTA);

    // Calculate average waiting time and print it in output file
    double Avg_waiting= (double) (total_waiting_time/processCount);
    fprintf(outCalcFile,"Avg Waiting = %.2f \n",Avg_waiting);

    // Calculate standard deviation and print it in output file
    StandardDeviation(outCalcFile);

    // Close the output log and calculations file
    fclose(outLogFile);
    fclose(outCalcFile);
    
    // Upon termination, release resources of communication with the clock module
    //destroyClk(false);

    return 0;
}

// Resumes a currently running process
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

// Starts a process for the first time
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
        ///////printf("Process created successfully.\n");

        // Print the "started" line in the output log file
        fprintf(outLogFile, "At time %d process %d started arr %d total %d remain %d wait %d\n", currTime, processPCB->id, processPCB->arrivalTime, processPCB->runTime, processPCB->remainingTime, processPCB->waitingTime);
    }
}

// Stops a currently running process
void stopProcess(PCB* processPCB, FILE* outLogFile, bool silent) {

    // Send a stop signal to the process
    kill(processPCB->pid, SIGSTOP);

    // Calculate and update the process remaining time and state
    printf("Inside stop id %d pid %d\n", processPCB->id, processPCB-> pid);
    int currTime = getClk();
    processPCB->remainingTime = (processPCB->runTime) -  (currTime - processPCB->arrivalTime - processPCB->waitingTime);
    processPCB->state = WAITING;

    // Print the "stopped" line in the output log file
    if(!silent) {
        fprintf(outLogFile, "At time %d process %d stopped arr %d total %d remain %d wait %d\n", currTime, processPCB->id, processPCB->arrivalTime, processPCB->runTime, processPCB->remainingTime, processPCB->waitingTime);
    }
}

// Updates the process data as appropriate upon termination
void finishProcess(PCB* processPCB, FILE* outLogFile)
{
    int currTime = getClk();

    // Calculate and update the process remaining time, finish time and state
    processPCB->remainingTime = 0;
    processPCB->finishTime = processPCB->arrivalTime + processPCB->waitingTime + processPCB->runTime;
    processPCB->state = FINISHED;

    // Calculate turnaround time and the weighted turnaround time
    double turn_around_time = currTime - processPCB->arrivalTime;
    double w_turn_around_time = turn_around_time/((double)(processPCB->runTime));

    // Calcualate contribution to the global variables
    total_waiting_time = total_waiting_time + processPCB->waitingTime;
    total_WTA = total_WTA + w_turn_around_time;
    total_proceesing_time = total_proceesing_time + processPCB->runTime;
    processCount++;
    double* WTApointer = (double *) malloc(sizeof(double));
    *WTApointer =  w_turn_around_time;
    enqueue(&WTAQueue, WTApointer);

    // Print the "finished" line in the output log file
    fprintf(outLogFile, "At time %d process %d finished arr %d total %d remain %d wait %d TA %.0f WTA %.2f\n", currTime, processPCB->id, processPCB->arrivalTime, processPCB->runTime, processPCB->remainingTime, processPCB->waitingTime, turn_around_time, w_turn_around_time);
    free(processPCB);
    processPCB = NULL;
    succesful_exit_handler = 0;
}

// Handler for the SIGUSR1 signal sent to scheduler upon successful termination of a process
void userHandler(int signum) {
    printf("Process successfully terminated.\n");
    succesful_exit_handler = 1;
}

// Calculates the standard deviation of the weighted turnaround times of all processes
void StandardDeviation(FILE* outCalcFile)
{
    // Average weighted turnaround time
    double AWTA = (double) (total_WTA/processCount);

    // Dequeuing from WTAQueue to calculate standard deviation
    double currDouble;
    double SD_numerator = 0;
    while (getQueueSize(&WTAQueue) != 0) {
        
        dequeue(&WTAQueue,&currDouble);
        double powerResult = pow((currDouble-AWTA), 2);
        SD_numerator += powerResult;
    }

    // Final result of the standard deviation of the weighted turnaround times
    double SD = sqrt(((double) (SD_numerator/processCount)));

    // Print the result in the output file
    fprintf(outCalcFile, "Std WTA = %.2f\n", SD);
}

// Shortest Remaining Time Next Algorithm
void SRTN(FILE* outLogFile) {

    printf("Running SRTN.\n");

    // Initialize a priority queue
    PNode* PQueueHead = NULL;
    struct msgbuff tempBuffer;
    PCB* tempPCB = NULL;
    int status;

    while(1) {

        ///////printf("Inside While\n");

        // In case of an empty priority queue
        if (isEmpty(&PQueueHead)) {

            // If the termination flag is 1; i.e. no more processes are coming
            if(finish_scheduler) {
                printf("Empty priority queue (%d) and finish flag is true\n", isEmpty(&PQueueHead));
                break;
            }

            // Wait till you receive a message 
            status = 0;
            while(!status) {
                printf("Will wait for message\n");
                tempBuffer = receiveMsg(1, &status);
            }

            if(status) {
                // Received message
                printf("Received process with id %d and pid %d\n", tempBuffer.data.id, tempBuffer.data.pid);

                // Check if incoming process is a flag
                if (tempBuffer.data.pid == -10)
                {
                    // Break
                    printf("Empty priority queue (%d) and received flag\n", isEmpty(&PQueueHead));
                    break;
                }
                else {
                    // If process is not flag
                    tempPCB = (PCB *) malloc(sizeof(PCB));  
                    equate(&tempBuffer.data, tempPCB); 
                    // Push process to the priority queue
                    push(&PQueueHead, tempPCB, tempPCB->remainingTime);
                    printf("Pushed process with id %d and pid %d\n", tempPCB->id, tempPCB->pid);
                }
            }
            else {
                printf("Empty priority queue and could not receive messsage\n");
            }           
        }

        status = 0;
        tempBuffer = receiveMsg(0, &status);
        while(status == 1) {
            
            printf("Received process with id %d and pid %d\n", tempBuffer.data.id, tempBuffer.data.pid);

            // Check for flag process
            if (tempBuffer.data.pid == -10)
            {
                finish_scheduler = 1;
                status = 0;
            }       
            else {
                // Not flag process
                tempPCB = (PCB *) malloc(sizeof(PCB));  
                equate(&tempBuffer.data, tempPCB); 
                // Push process to the priority queue
                push(&PQueueHead, tempPCB, tempPCB->remainingTime);
                printf("Pushed process with id %d and pid %d\n", tempPCB->id, tempPCB->pid);
                status = 0;
                tempBuffer = receiveMsg(0, &status);
            }
        }

        PCB* currProcessPCB = (PCB *) malloc(sizeof(PCB));      
        int afterPop = pop(&PQueueHead, currProcessPCB);
        
        printf("Successful popping (%d). Popped process with id %d and pid %d\n", afterPop, currProcessPCB->id, currProcessPCB->pid);

        // Chech whether process was forked before
        if (currProcessPCB->pid == -5) {
            // Start process
            startProcess(currProcessPCB, outLogFile);
            printf("Started process with id %d and pid %d\n", currProcessPCB->id, currProcessPCB->pid);
        }
        else {
            // Resume process
            resumeProcess(currProcessPCB, outLogFile, 0);
        }

        succesful_exit_handler = 0;

        // If termination flag is not set yet; i.e. more processes are still coming        
        if(!finish_scheduler) {
            printf("Will wait for message\n");
            status = 0;
            while(!status) {
                tempBuffer = receiveMsg(1, &status);
            }
        }
        else {
            // No more processes are coming
            // Wait for current process to terminate
            int stat;
            pid_t isCurrProccess = waitpid(currProcessPCB->pid, &stat, 0);
            if ((isCurrProccess != currProcessPCB->pid) || !(WIFEXITED(stat))) {
                printf("Exit code received from process: %d.\n", WEXITSTATUS(stat));
                if (WIFSIGNALED(stat)) {
                    psignal(WTERMSIG(stat), "Exit signal:");  
                }       
            }
            ///////printf("Exit status from process %d.\n", WIFEXITED(stat));
            ///////printf("After sleep\n");
        }

        // If running process exits successfully
        if (succesful_exit_handler) {

            printf("Will finish process with id %d\n", currProcessPCB->id);
            finishProcess(currProcessPCB, outLogFile);
            printf("Finished process with id %d\n", currProcessPCB->id);
        }
        else if (status == 1) {

            // A message was received
            // Check for flag process
            if (tempBuffer.data.pid != -10) {
                // Stop running process
                stopProcess(currProcessPCB, outLogFile, 0);
                succesful_exit_handler = 0;
                tempPCB = (PCB *) malloc(sizeof(PCB));  
                equate(&tempBuffer.data, tempPCB);    
                // Push both processes into the priority queue
                push(&PQueueHead, currProcessPCB, currProcessPCB->remainingTime);
                push(&PQueueHead, tempPCB, tempPCB->remainingTime);
                printf("Pushed process with id %d and pid %d\n", tempPCB->id, tempPCB->pid);
            }
            else {
                // Stop and resume running process just to correctly calculate all its attributes
                stopProcess(currProcessPCB, outLogFile, 0);
                succesful_exit_handler = 0;
                resumeProcess(currProcessPCB, outLogFile, 0);
                printf("Will wait for process with pid %d to finish\n", currProcessPCB->pid);

                // Wait for process to finish
                int stat;
                pid_t isCurrProccess = waitpid(currProcessPCB->pid, &stat, 0);
                if ((isCurrProccess != currProcessPCB->pid) || !(WIFEXITED(stat))) {
                    printf("Exit code received from process: %d.\n", WEXITSTATUS(stat));
                    if (WIFSIGNALED(stat)) {
                        psignal(WTERMSIG(stat), "Exit signal:");  
                    }       
                }
                ///////printf("Exit status from process %d.\n", WIFEXITED(stat));
                ///////printf("After sleep\n");

                finishProcess(currProcessPCB, outLogFile);
                printf("Finished process with id %d\n", currProcessPCB->id);

                // Set termination flag
                finish_scheduler = 1;
            }
        }
    }
    return;
}

void RR(FILE* outLogFile, int Quantum) {

    printf("Running RR\n");

    // Initializing ready queue
    Queue readyQueue;
    queueInit(&readyQueue, sizeof(PCB));
    struct msgbuff tempBuffer;
    PCB* tempPCB = NULL;
    int status;
    finish_scheduler = 0;

    while(1) {
        ///////printf("Inside While\n");

        // If queue is empty 
        if (getQueueSize(&readyQueue) == 0) {


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
    }
    return;
}