#include <stdio.h>    // puts(), printf(), perror(), getchar()
#include <stdlib.h>   // exit(), EXIT_SUCCESS, EXIT_FAILURE
#include <unistd.h>   // getpid(), getppid(),fork()
#include <sys/wait.h> // wait()

#define READ  0
#define WRITE 1

void child_a(int fd[]) {
    dup2(fd[WRITE], STDOUT_FILENO); // replace stdout with fd[write] which writes to the pipeline
    execlp("ls", "ls", "-F", "-1", NULL);
    close(fd[WRITE]);
    exit(getpid());
}

void child_b(int fd[]) {
    dup2(fd[READ], STDIN_FILENO); // replace stdin with fd[read] which reads from the pipeline
    execlp("nl", "nl", NULL);
    close(fd[READ]);
    exit(getpid());
}

int main(void) {
    int fd[2];
    pipe(fd);
    int id = fork();
    if (id < 0) {
        printf("First fork failed");
        exit(EXIT_FAILURE);
    } else if (id > 0) {
        int id2 = fork();
        if (id2 < 0) {
            printf("Second fork failed");
            exit(EXIT_FAILURE);
        }
        close(fd[WRITE]);
        if (id2 == 0) {
            child_b(fd);
        }
    } else {
        child_a(fd);
    }

    if (id != 0) {
        wait(NULL);
        wait(NULL);
    }
}
