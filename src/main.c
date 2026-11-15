#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

        while ((cmdline = readline(PROMPT)) != NULL) {
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
            add_history(cmdline);
        }
        if ((arglist = tokenize(cmdline)) != NULL) {
                        if (handle_builtin(arglist) == 0) {
                execute(arglist);
            }

            // Free the memory allocated by tokenize()
            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }
            free(arglist);
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
