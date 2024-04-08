#include "parser.h"    // cmd_t, position_t, parse_commands()

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>     //fcntl(), F_GETFL

#define READ  0
#define WRITE 1

/**
 * For simplicitiy we use a global array to store data of each command in a
 * command pipeline .
 */
cmd_t commands[MAX_COMMANDS];
bool multiple_commands = false;

/**
 *  Debug printout of the commands array.
 */
void print_commands(int n) {
    for (int i = 0; i < n; i++) {
        printf("==> commands[%d]\n", i);
        printf("  pos = %s\n", position_to_string(commands[i].pos));
        printf("  in  = %d\n", commands[i].in);
        printf("  out = %d\n", commands[i].out);

        print_argv(commands[i].argv);
    }

}

/**
 * Returns true if file descriptor fd is open. Otherwise returns false.
 */
int is_open(int fd) {
    return fcntl(fd, F_GETFL) != -1 || errno != EBADF;
}

void fork_error() {
    perror("fork() failed)");
    exit(EXIT_FAILURE);
}

/**
 *  Fork a proccess for command with index i in the command pipeline. If needed,
 *  create a new pipe and update the in and out members for the command..
 */
void fork_cmd(int i, int fd[], int n) {
    pid_t pid;
    switch (pid = fork()) {
        case -1:
            fork_error();
        case 0:
            if (n > 1 && commands[i].pos == first) {
                close(fd[READ]);
                dup2(fd[WRITE], STDOUT_FILENO);
            } else if (n > 1 && commands[i].pos == last) {
                close(fd[WRITE]);
                dup2(fd[READ], STDIN_FILENO);
            } else if (n > 1 && commands[i].pos == middle) {
                dup2(fd[READ], STDIN_FILENO);
                dup2(fd[WRITE], STDOUT_FILENO);
            }
            // Execute the command in the contex of the child process.
            execvp(commands[i].argv[0], commands[i].argv);
            // If execvp() succeeds, this code should never be reached.
            fprintf(stderr, "shell: command not found: %s\n", commands[i].argv[0]);
            exit(EXIT_FAILURE);

        default:
        // Parent process after a successful fork().
        if (n == 2) {
            close(fd[WRITE]);
        }
            break;
    }
}

/**
 *  Fork one child process for each command in the command pipeline.
 */
void fork_commands(int n) {

    if (n > 1) {
        int fd[2];
        int result = pipe(fd);
        if (result < 0) {
            printf("pipe error!\n");
        }
        for (int i = 0; i < n; i++) {
            fork_cmd(i, fd, n);
        }
    } else {
        for (int i = 0; i < n; i++) {
            fork_cmd(i, 0, n);
        }
    }
}

/**
 *  Reads a command line from the user and stores the string in the provided
 *  buffer.
 */
void get_line(char* buffer, size_t size) {
    ssize_t lines_read = getline(&buffer, &size, stdin);
    if (lines_read < 0) {
        printf("Error in getline, or EOF has been reached.");
    }
    buffer[strlen(buffer)-1] = '\0';
}

/**
 * Make the parents wait for all the child processes.
 */
void wait_for_all_cmds(int n) {
    for (int i = 0; i < n; i++) {
        pid_t pid = wait(NULL);
        if (pid < 0) {
            printf("Error in wait");
        }
    }
}

int main() {
    int n;               // Number of commands in a command pipeline.
    size_t size = 128;   // Max size of a command line string.
    char line[size];     // Buffer for a command line string.


    while(true) {
        printf(" >>> ");

        get_line(line, size);

        n = parse_commands(line, commands);

        //print_commands(n);

        fork_commands(n);

        wait_for_all_cmds(n);
    }

    exit(EXIT_SUCCESS);
}
