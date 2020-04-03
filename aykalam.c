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

    // Reading the main arguments 
    int quantum;
    //char shedalg[5];
    char *schedalg = NULL;
    if (argc == 2) {
        
        // Get the chosen algorithm 
        schedalg = argv[0];
        printf("Alg: %s\n", schedalg); 

        // Get the quantum needed for round robin algorithm 
        quantum = atoi(argv[1]);
        printf("Quantum %d.\n", quantum);

    }
    ///////////////////////
    // Deciding which algorithm that the process generator sent 
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
/////////////////////////////////
    initMsgQueue();

    // Close the output log and calculations file
    fclose(outLogFile);
    fclose(outCalcFile);
    
    // Upon termination, release resources of communication with the clock module
    //destroyClk(false);

    return 0;
}