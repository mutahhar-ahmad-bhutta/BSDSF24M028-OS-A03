#include "shell.h"
#include <fcntl.h>

// Feature 5: look for < and > in the arguments and redirect stdin/stdout.
// This runs ONLY in the child process, after fork() and before execvp().
void setup_redirection(char** args) {
    char* infile = NULL;
    char* outfile = NULL;
    int first_op = -1;      // position of the first < or >

    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0) {
            if (args[i + 1] == NULL) {
                fprintf(stderr, "syntax error: missing file name after %s\n", args[i]);
                exit(1);
            }
            if (args[i][0] == '<') infile = args[i + 1];
            else                   outfile = args[i + 1];
            if (first_op == -1) first_op = i;
            i++;            // skip the file name
        }
    }

    if (infile != NULL) {
        int fd = open(infile, O_RDONLY);
        if (fd < 0) { perror(infile); exit(1); }
        dup2(fd, STDIN_FILENO);     // stdin now comes from the file
        close(fd);
    }
    if (outfile != NULL) {
        int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) { perror(outfile); exit(1); }
        dup2(fd, STDOUT_FILENO);    // stdout now goes to the file
        close(fd);
    }

    // cut the command at the first < or >, so execvp only sees the command
    if (first_op != -1) args[first_op] = NULL;
}

// Feature 5: run "left | right"
int execute_pipe(char** arglist, int pipe_pos) {
    int fd[2];                       // fd[0] = read end, fd[1] = write end
    if (pipe(fd) == -1) { perror("pipe"); return 1; }

    int left = fork();
    if (left == 0) {                 // left child: the WRITER
        arglist[pipe_pos] = NULL;    // left command ends at the |
        dup2(fd[1], STDOUT_FILENO);  // stdout goes into the pipe
        close(fd[0]);
        close(fd[1]);
        setup_redirection(arglist);
        execvp(arglist[0], arglist);
        perror("Command not found");
        exit(1);
    }

    int right = fork();
    if (right == 0) {                // right child: the READER
        char** right_args = &arglist[pipe_pos + 1];
        dup2(fd[0], STDIN_FILENO);   // stdin comes from the pipe
        close(fd[0]);
        close(fd[1]);
        setup_redirection(right_args);
        execvp(right_args[0], right_args);
        perror("Command not found");
        exit(1);
    }

    // parent: close BOTH ends, otherwise the reader never sees end-of-file
    close(fd[0]);
    close(fd[1]);
    waitpid(left, NULL, 0);
    waitpid(right, NULL, 0);
    return 0;
}

int execute(char* arglist[]) {
    // Feature 5: is there a pipe?
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "|") == 0) {
            if (i == 0 || arglist[i + 1] == NULL) {
                fprintf(stderr, "syntax error near |\n");
                return 1;
            }
            return execute_pipe(arglist, i);
        }
    }

    int status;
    int cpid = fork();

    switch (cpid) {
        case -1:
            perror("fork failed");
            exit(1);
        case 0: // Child process
            setup_redirection(arglist);   // Feature 5
            execvp(arglist[0], arglist);
            perror("Command not found"); // This line runs only if execvp fails
            exit(1);
        default: // Parent process
            waitpid(cpid, &status, 0);
            return 0;
    }
}
