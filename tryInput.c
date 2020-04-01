#include <stdio.h>
#include <stdlib.h>
#include "Queue.h"
#include"PCB.h"


int main()
{
    Queue *q;
    PCB myPCB1;
    //PCB * myPCB1 = (PCB *) malloc(sizeof(PCB));
    myPCB1.arrivalTime = 3;
    myPCB1.id = 1;
    myPCB1.priority = 2;
    myPCB1.runTime = 6;

    PCB myPCB2;
    myPCB2.arrivalTime = 5;
    myPCB2.id = 8;
    myPCB2.priority = 7;
    myPCB2.runTime = 9;

    PCB myPCB3;
    myPCB3.arrivalTime = 13;
    myPCB3.id = 15;
    myPCB3.priority = 42;
    myPCB3.runTime = 57;

    PCB deqPCB;
    PCB peekPCB;


    queueInit(q, sizeof(PCB));

    enqueue(q, &myPCB1);
    enqueue(q, &myPCB2);
    enqueue(q, &myPCB3);

    
    
    
    dequeue(q, &deqPCB);


    //dequeue(&q, &deqPCB);
    printf("The deqPCB arrival time is %d\n", deqPCB.arrivalTime);
    printf("The deqPCB ID is %d\n", deqPCB.id);
    printf("The deqPCB priority is %d \n", deqPCB.priority);
    printf("The deqPCB run time is %d \n\n\n", deqPCB.runTime);

    

    /*dequeue(&q, &deqPCB);
    printf("The deqPCB arrival time is %d\n", deqPCB.arrivalTime);
    printf("The deqPCB ID is %d\n", deqPCB.id);
    printf("The deqPCB priority is %d \n", deqPCB.priority);
    printf("The deqPCB run time is %d \n\n\n", deqPCB.runTime);*/

    int size = getQueueSize(q);
    printf("The Queue size  is %d \n\n\n", size);

    clearQueue(q);

    queuePeek(q,&peekPCB);
    printf("The peekPCB arrival time is %d\n", peekPCB.arrivalTime);
    printf("The peekPCB ID is %d\n", peekPCB.id);
    printf("The peekPCB priority is %d \n", peekPCB.priority);
    printf("The peekPCB run time is %d \n\n\n", peekPCB.runTime);

    int size2 = getQueueSize(q);
    printf("The Queue size 2  is %d \n\n\n", size2);


    return 0;
}