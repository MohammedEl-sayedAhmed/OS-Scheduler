#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include "headers.h"
#include "PCB.h"
#include "Queue.h"

//comment:I make -key variable- generic ..I am not sure about how much this is goog.I made this to let both fns Send_msg  && clearResources see it .
void clearResources(int signum);
key_t key=13245; 

void readInputFile(Queue* arrivedProcessesQueue);

pid_t createScheduler(char * const * argv);
pid_t createClock();
void sendProcessAtAppropTime (Queue* arrivedProcessesQueue);

int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    Queue* arrivedProcessesQueue;

    // 1. Read the input files.
    readInputFile(arrivedProcessesQueue);

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.

    //SRTN shortest remaining time next , HPF -heighest priority first -non permitive- , RR Round Robin

    char  scheduling_algorithm[5];
    int Quantum;
    
    printf("SRTN,HPF,RR are the available scheduling algorithms ,choose one of them \n");
    scanf ("%s",scheduling_algorithm);
    
    while(strcmp(scheduling_algorithm,"RR")!=0 && strcmp(scheduling_algorithm,"STRN")!=0 && strcmp(scheduling_algorithm,"HPF")!=0)
    {
        printf ("Error ,You must choose one of the provided algorithms only \n");
        printf("SRTN,HPF,RR are the available scheduling algorithms ,choose one of them \n");
        scanf ("%s",scheduling_algorithm);
    }
    if(strcmp(scheduling_algorithm, "RR") == 0)
    {
        printf ("Enter Quantum \n");
        scanf("%d",&Quantum);
    }

    char QuantumStr[100];
    sprintf(QuantumStr, "%d", Quantum); ///// etcommittyyy b2aa


    // 3. Initiate and create the scheduler and clock processes.
    // define parameter list

    char * param[] = {"scheduler", scheduling_algorithm, QuantumStr , NULL};
    pid_t schedulerPID = createScheduler(param);
    createClock();

    // 4. Use this function after creating the clock process to initialize clock
    initClk();


    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.

    // 6. Send the information to the scheduler at the appropriate time.
    sendProcessAtAppropTime (arrivedProcessesQueue);
    // send to scheduler
    //Send_msg(struct msgbuff message); // will be fixed 

    // 7. Clear clock resources
    destroyClk(true);
}


//message buffer :
struct msgbuff
{
    long mtype;
    struct PCB* data;
};

//Sending messages function :
void Send_msg(struct PCB * pointer_1)
{    
    //Creat resource -mailbox- (queue of messages)
    int msgqid = msgget(key,IPC_CREAT| 0644); // or msgget(12613, IPC_CREAT | 0644)
    //Making sure from validity of creation of resource
    if(msgqid == -1)
    perror("Invalid up_msgid");
    else
    printf("msgid=%d", msgqid);
    pid_t Pid=getpid();    
    //comment:Not sure if this is right or wrong !    
    struct msgbuff message;   
    message.mtype =Pid;  
    message.data=pointer_1;
    
    int  send_val ;
    //comment:Need to make sure from this -(!IPC_NOWAIT) or (IPC_NOWAIT)-
    send_val = msgsnd(msgqid , &message,sizeof(message.data), !IPC_NOWAIT);

        if(send_val == -1)
            perror("Errror in send");
        else 
            printf("sent already");
}

//TODO Clears all resources in case of interruption
//clearResources function :
void clearResources(int signum)
{
    printf("Resources are cleared now..\n");
    int x = msgget(key,0644);
    msgctl(x, IPC_RMID, (struct msqid_ds *) 0);
    exit(0);
}

//1. Read the input files.
void readInputFile(Queue* arrivedProcessesQueue)
{
    FILE *inputFile; 
    inputFile = fopen("processes.txt", "r"); // open the input file in a read mode
    char intputFileLine[20]; // assign the lines in the txt file to intputFileLine

   while (!feof(inputFile) ) {

        intputFileLine[0] = fgetc(inputFile); // fgetc() is used to obtain input from a file single character at a time
        
        if (intputFileLine[0] != '#'){ // if it is not #, then it is a process

            // this is a process -- save it in whatever stuff (will be a queue later)
            PCB * myNewPCB = (PCB *) malloc(sizeof(PCB)); // malloc >> memory allocation, it is like new in c++
          
            // save the ID first, because there is a bug that makes the ID= 0 many times so we want to check for it
            // Also use atoi; to convert the string to int -- like %d in fscanf 
            myNewPCB->id = atoi(intputFileLine); 

            if (myNewPCB->id == 0){
                continue; // to skip the 0 process id that comes in between
            }

            fscanf(inputFile , "%d %d %d", &myNewPCB->arrivalTime, &myNewPCB->runTime , &myNewPCB->priority); // read the process data and save it proberly to the pcb struct object

            /*printf("The ID is %d\n", myNewPCB->id);
            printf("The arrival time is %d\n", myNewPCB->arrivalTime);
            printf("The run time is %d \n", myNewPCB->runTime);
            printf("The priority is %d \n\n\n", myNewPCB->priority);*/
            
            // enqueue myNewPCB in arrivedProcessesQueue
            queueInit(&arrivedProcessesQueue, sizeof(PCB));
            enqueue(&arrivedProcessesQueue , myNewPCB);
        }
        else{ // malhash lazma, just 7antafa 

            char notProcessData[200];
            fgets(notProcessData, sizeof(notProcessData), inputFile);  
        }
    } 
    fclose(inputFile);
}

pid_t createScheduler(char * const * argv){

    pid_t schedulerPID = fork();

    if (schedulerPID == -1){

        perror("Error in scheduler fork\n");
        exit(1);
    }

    else if (schedulerPID == 0){

         printf("creating scheduler process\n");
        int forkResult;
        forkResult = execv("./scheduler.out", argv); // argv will be the paramter list defined in the main
        
        if (forkResult == -1){
            printf("Failed to create scheduler process\n");
        }
        else if(forkResult == 0){
            printf("Scheduler process created successfully!! \n");
        }
        /*else if (forkResult == 1){
            printf("Scheduler Parent!! \n");
        }*/
    }
    //
    return schedulerPID;  
}



pid_t createClock(){
    printf("trying to  clock process\n");

    pid_t clockPID;
    clockPID = fork();

    if (clockPID == -1){

        perror("Error in Clock fork\n");
        exit(1);
    }
    else if (clockPID == 0){

        printf("creating clock process\n");

        int forkResult;
        forkResult = execvp("./clk.out",NULL); // or execv not sure witt try it when it works 
        if (forkResult == -1){
            printf("Failed to create clock process\n");
        }
        else if (forkResult == 0){
            printf("Clock process created successfully!! \n");
        }
    }
    //sleep(0.5); // kont ba5leha t sleep 3shan kan mara ysht3'al w mara la2 f kont 3ayz astna showaya 3shan yban el forking bta3 el sched
    printf("will retrun clockPID\n");
    //printf("The clkpID is %d\n", clockPID);
    return clockPID;  
}


void sendProcessAtAppropTime (Queue* arrivedProcessesQueue){

	while (getQueueSize(&arrivedProcessesQueue) != 0){
        //printf("The sizeeeeeeeeeeeeeeee is %d\n", getQueueSize(&arrivedProcessesQueue));

        PCB* arrivedProcess;
        dequeue(&arrivedProcessesQueue,&arrivedProcess);
		int currTime = getClk();
        //int currTime = 10;
        printf("current time is %d\n", currTime);

		int currProcessArrTime = arrivedProcess->arrivalTime;
		int sleepTime = currProcessArrTime - currTime;
		sleep(sleepTime);

	}
}
