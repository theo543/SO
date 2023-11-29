#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

const char *LS_LOCATION = "/usr/bin/ls";

int main(int argc, char **argv, char **envp) {
    (void)argc;
    (void)argv;

    pid_t pid = fork();
    if(pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    } else if(pid == 0) {
        // child
        char *argv[] = {"ls", NULL};
        execve(LS_LOCATION, argv, envp);
        perror("execve");
        return EXIT_FAILURE;
    } else {
        // parent
        printf("My PID=%d, Child PID=%d\n", getpid(), pid);
        if(wait(NULL) == -1) {
            perror("waitpid");
            return EXIT_FAILURE;
        }
        printf("Child %d finished\n", pid);
    }
}
