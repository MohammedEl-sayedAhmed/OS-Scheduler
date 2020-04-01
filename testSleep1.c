#include "headers.h"
#include <signal.h>

//void handler(int signal) {
//}
int main(int argc, char * argv[]) {

    //signal(SIGINT, handler);
    printf("%d\n",argc);
    if (argc == 2) {

        // Set the process running time to equal that passed to it
        int quantum = atoi(argv[1]);
        printf("Quantum %d.\n", quantum);

        // Initialize the remaining time with the running time
        char *shedalg = argv[0];
        printf("Alg: %s\n", shedalg);
    }
    printf("Inside testSleep1\n");

    //sleep(20);
    //printf("after sleep\n");
    return 0;
}
