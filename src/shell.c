#include "shell.h"

char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);
    char* cmdline = (char*) malloc(sizeof(char) * MAX_LEN);
    int c, pos = 0;

    while ((c = getc(fp)) != EOF) {
        if (c == '\n') break;
        cmdline[pos++] = c;
    }

    if (c == EOF && pos == 0) {
        free(cmdline);
        return NULL; // Handle Ctrl+D
    }
    
    cmdline[pos] = '\0';
    return cmdline;
}

char** tokenize(char* cmdline) {
    // Edge case: empty command line
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n') {
        return NULL;
    }

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
        bzero(arglist[i], ARGLEN);
    }

    char* cp = cmdline;
    char* start;
    int len;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++; // Skip leading whitespace
        
        if (*cp == '\0') break; // Line was only whitespace

        start = cp;
        len = 1;
        while (*++cp != '\0' && !(*cp == ' ' || *cp == '\t')) {
            len++;
        }
        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
    }

    if (argnum == 0) { // No arguments were parsed
        for(int i = 0; i < MAXARGS + 1; i++) free(arglist[i]);
        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}
// Feature 2: built-in commands
int handle_builtin(char** arglist) {
    if (strcmp(arglist[0], "exit") == 0) {
        printf("Bye!\n");
        exit(0);
    }
    else if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) {
            fprintf(stderr, "cd: missing directory\n");
        } else if (chdir(arglist[1]) != 0) {
            perror("cd");
        }
        return 1;
    }
    else if (strcmp(arglist[0], "history") == 0) {
        show_history();
        return 1;
    }
    else if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd <dir>  : change directory\n");
        printf("  exit      : exit the shell\n");
        printf("  help      : show this help\n");
        printf("  jobs      : show background jobs\n");
        printf("  history   : show command history\n");
        printf("  !n        : run command number n from history\n");
        return 1;
    }
    else if (strcmp(arglist[0], "jobs") == 0) {
        printf("Job control not yet implemented.\n");
        return 1;
    }
    return 0;
}

// Feature 3: command history
char* history[HISTORY_SIZE];   // stores the commands
int history_count = 0;         // total commands typed so far

void add_to_history(char* cmd) {
    int index = history_count % HISTORY_SIZE;
    if (history_count >= HISTORY_SIZE) {
        free(history[index]);          // array is full: remove the oldest
    }
    history[index] = strdup(cmd);      // save a copy of the command
    history_count++;
}

void show_history() {
    int start = 0;
    if (history_count > HISTORY_SIZE) {
        start = history_count - HISTORY_SIZE;
    }
    for (int i = start; i < history_count; i++) {
        printf("%d  %s\n", i + 1, history[i % HISTORY_SIZE]);
    }
}

char* get_history(int n) {
    int start = 0;
    if (history_count > HISTORY_SIZE) {
        start = history_count - HISTORY_SIZE;
    }
    if (n <= start || n > history_count) {
        return NULL;                   // n is out of range
    }
    return history[(n - 1) % HISTORY_SIZE];
}
