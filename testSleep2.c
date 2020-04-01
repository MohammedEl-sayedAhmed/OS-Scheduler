#include "headers.h"

int main() {

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
    return 0;
}
