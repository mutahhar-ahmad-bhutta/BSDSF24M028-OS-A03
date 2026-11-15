#include "shell.h"

// Run one full line: split at ; , handle & , built-ins and external commands.
// Returns the exit status of the last command (0 = success).
int run_line(char* line) {
    int last_status = 0;
    char** arglist;
    char* copy = strdup(line);          // strtok_r changes the string, so use a copy
    char* saveptr;
    char* part = strtok_r(copy, ";", &saveptr);

        while (part != NULL) {
        // Feature 8: NAME=value is handled by the shell itself (no fork)
        if (is_assignment(part)) {
            do_assignment(part);
            last_status = 0;
            part = strtok_r(NULL, ";", &saveptr);
            continue;
        }

        if ((arglist = tokenize(part)) != NULL) {
            expand_variables(arglist);          // Feature 8: $NAME -> value
            // Feature 6: is the last word & ?
            int background = 0;
            int last = 0;
            while (arglist[last + 1] != NULL) last++;
            if (strcmp(arglist[last], "&") == 0) {
                background = 1;
                free(arglist[last]);
                arglist[last] = NULL;
            }

            if (arglist[0] != NULL) {
                if (handle_builtin(arglist) == 1) {
                    last_status = 0;                    // built-ins count as success
                } else {
                    last_status = execute(arglist, background);
                }
            }

            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }
            free(arglist);
        }
        part = strtok_r(NULL, ";", &saveptr);
    }
    free(copy);
    return last_status;
}

// Feature 7: read and run an if-then-else-fi block.
// first_line is the line that starts with "if ".
void run_if_block(char* first_line) {
    char* then_lines[50];
    char* else_lines[50];
    int then_count = 0, else_count = 0;
    int mode = 0;                       // 0 = waiting for then, 1 = in then, 2 = in else

    char* condition = strdup(first_line + 3);   // the command after "if "

    while (1) {
        char* line = readline("> ");    // keep reading until fi
        if (line == NULL) {             // Ctrl+D before fi
            printf("syntax error: missing fi\n");
            break;
        }
        char* word = line;
        while (*word == ' ' || *word == '\t') word++;   // skip spaces

        if (strcmp(word, "then") == 0)      mode = 1;
        else if (strcmp(word, "else") == 0) mode = 2;
        else if (strcmp(word, "fi") == 0) {
            free(line);
            // run the condition, then choose a block based on its exit status
            int status = run_line(condition);
            if (status == 0) {
                for (int i = 0; i < then_count; i++) run_line(then_lines[i]);
            } else {
                for (int i = 0; i < else_count; i++) run_line(else_lines[i]);
            }
            break;
        }
        else if (mode == 1 && then_count < 50) then_lines[then_count++] = strdup(word);
        else if (mode == 2 && else_count < 50) else_lines[else_count++] = strdup(word);
        free(line);
    }

    for (int i = 0; i < then_count; i++) free(then_lines[i]);
    for (int i = 0; i < else_count; i++) free(else_lines[i]);
    free(condition);
}

int main() {
    char* cmdline;
    rl_variable_bind("enable-bracketed-paste", "off");   // pasted lines arrive one by one

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

        // Feature 7: if-then-else-fi
        if (strncmp(cmdline, "if ", 3) == 0) {
            run_if_block(cmdline);
        } else {
            run_line(cmdline);
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
