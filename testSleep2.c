#include "headers.h"
int SRTN(FILE* outLogFile) {

//    printf("Inside SRTN\n");
    PNode* PQueueHead = NULL;
    struct msgbuff tempBuffer;
    PCB tempPCB;
    int status;
    //bool silence = false; //////////

    while (exitnow!=1) {  ////te2felllll + silent in file or not

//        printf("Inisde while 1");
        //if ((isEmpty(&PQueueHead)) || (!isEmpty(&PQueueHead))) {
        if (isEmpty(&PQueueHead)) {

            if(finish_scheduler) {
//                printf("will break right -10 is empty%d\n", isEmpty(&PQueueHead));
                exitnow = 1;
                continue;
            }

            tempBuffer = receiveMsg(1, &status);
//            printf("Status after rec %d\n", status);
            //if(status) {
            //    printf("pcb pid bef %d", tempBuffer.data.arrivalTime);
            //}

            if(status) {
                equate(&tempBuffer.data, &tempPCB);    
                //tempPCB = tempBuffer.data;
//                printf("pcb pid %d", tempPCB.pid);
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

int main() {

    /*
    int pid = fork();

    if (pid == -1)
        perror("Error in fork. Could not start process.\n");

    else if (pid == 0)
    {
        printf("forked\n");
        char *argv[] = { "./clk.out", 0};
        execve(argv[0], &argv[0], NULL);
    }
    else {
        sleep(5);
        initClk();

        printf("Done test\n");
        destroyClk(true);

        sleep(5);

        kill(pid, SIGINT);
    }
    return 0;*/


}
