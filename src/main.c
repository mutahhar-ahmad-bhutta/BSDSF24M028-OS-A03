#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    while (1) {
        reap_jobs();                          // Feature 6: clean up zombies
        cmdline = readline(PROMPT);
        if (cmdline == NULL) break;           // Ctrl+D

        // Feature 3: !n re-execution
        if (cmdline[0] == '!') {
            int n = atoi(cmdline + 1);
            char* old_cmd = get_history(n);
            if (old_cmd == NULL) {
                printf("No such command in history.\n");
                free(cmdline);
                continue;
            }
            free(cmdline);
            cmdline = strdup(old_cmd);
            printf("%s\n", cmdline);
        }

        // Feature 3: save the command in history
        if (cmdline[0] != '\0') {
            add_to_history(cmdline);
            add_history(cmdline);
        }

        // Feature 6: split the line at ; and run each part in order
        char* saveptr;
        char* part = strtok_r(cmdline, ";", &saveptr);
        while (part != NULL) {
            if ((arglist = tokenize(part)) != NULL) {
                // Feature 6: is the last word & ?
                int background = 0;
                int last = 0;
                while (arglist[last + 1] != NULL) last++;
                if (strcmp(arglist[last], "&") == 0) {
                    background = 1;
                    free(arglist[last]);
                    arglist[last] = NULL;
                }

                if (arglist[0] != NULL && handle_builtin(arglist) == 0) {
                    execute(arglist, background);
                }

                // Free the memory allocated by tokenize()
                for (int i = 0; arglist[i] != NULL; i++) {
                    free(arglist[i]);
                }
                free(arglist);
            }
            part = strtok_r(NULL, ";", &saveptr);
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
