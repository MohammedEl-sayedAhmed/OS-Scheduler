#include <stdio.h>      //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "PCB.h"

typedef short bool;
#define true 1
#define false 1

#define SHKEY 300


///==============================
//don't mess with this variable//
int * shmaddr;                 //
//===============================



int getClk()
{
    return *shmaddr;
}


/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *) shmat(shmid, (void *)0, 0);
}


/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}

///////////MESSAGE QUEUE///////////

// The key of the one-way message queue between the process generator and the scheduler
int msgqid;

struct msgbuff {
    long mtype;
///////comment :immm ,I need to make sure from being (data) a pointer .
    PCB data;
};

void initMsgQueue() {

    // The same key as that specified in the process generator
    msgqid = msgget(13245, IPC_CREAT | 0644);

    // Making sure from the validity of msgqid
    if (msgqid == -1) {
        perror("Error in creating/getting message queue.\n");
        exit(-1);
    }
    printf("Message queue with msgqid = %d\n", msgqid);
}

void destroyMsgQueue() {

    printf("Deleting message queue.\n");
    int rem_msg = msgctl(msgqid, IPC_RMID, (struct msqid_ds *) 0);

    // Making sure of successful deletion
    if (rem_msg == -1) {
        perror("Error in deleting message queue.\n");
    }
    else {
        printf("Message queue deleted successfully.\n");
    }
}

bool sendMsg(PCB pointer_1)
{    
    //pid_t Pid = getpid();    
    
    //comment:Not sure if this is right or wrong !    
    struct msgbuff message;   
    message.mtype = 1;
    message.data = pointer_1;
    
    int send_val;
    //comment:Need to make sure from this -(!IPC_NOWAIT) or (IPC_NOWAIT)-
    send_val = msgsnd(msgqid , &message, sizeof(message.data), !IPC_NOWAIT);

    if(send_val == -1) {
        perror("Errror in send.\n");
        return false;
    }
    else {
        printf("Sent already.\n");
        return true;
    }
}

bool receiveMsg(int waitFlag, struct msgbuff* message)
{   
    // Create an object of the message buffer to receive the message in
    int rec_val;

    // Receive message irrespective of its type
    if (waitFlag) {
        rec_val = msgrcv(msgqid, message, sizeof(message->data), 0, !IPC_NOWAIT);
    }
    else {
        rec_val = msgrcv(msgqid, message, sizeof(message->data), 0, IPC_NOWAIT);
    }

    if(rec_val == -1) {
        perror("Error in receiving message.\n");
        return false;
    }
    else {
        ///////comment:print (For example)...mtype,startTime,priority of a message .
        printf("\nMessage type received: %ld \n", message->mtype);
    }

    return true;
}