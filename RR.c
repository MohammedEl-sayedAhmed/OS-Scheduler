#include <stdio.h>
#include <stdlib.h>
#include "Queue.h"
#include"PCB.h"


void RR(FILE* outLogFile, int Quantum) {


    Queue RRreadyQueue;
    queueInit(&RRreadyQueue, sizeof(PCB));
    int size = getQueueSize(&RRreadyQueue);
    struct msgbuff message;
    PCB* temp_process_pcb = NULL;
    PCB* ready_process_pcb = NULL;

    
    //printf("finish scheduler %d   ", finish_scheduler);
    while (1)
    {
        //printf("finish scheduler %d   ", finish_scheduler);
        if (getQueueSize(RRreadyQueue) == 0)
        {
            //printf("finish scheduler %d   ", finish_scheduler);
            /*if (finish_scheduler == true)
            {
                printf("HPPPPPPPPPFFFFFFF");
                break;
            }*/
            receiveMsg(1, message);
            temp_process_pcb = &(message.data);
            /*if (temp_process_pcb->pid == -10)
            {
                printf("true1/n");
                finish_scheduler = true;
                break;
            }*/
            enqueue(&RRreadyQueue, temp_process_pcb);

        }

        while (receiveMsg(0, message))
        {
            temp_process_pcb = &(message.data);
            /*if (temp_process_pcb->pid == -10)
            {
                printf("true1/n");
                finish_scheduler = true;
            }*/
            enqueue(&RRreadyQueue, temp_process_pcb);
        }

        if (getQueueSize(RRreadyQueue) != 0)
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