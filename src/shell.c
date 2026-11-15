#include "shell.h"
#include <ctype.h>

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

        // Feature 5: remove double quotes, so  grep "root"  searches for root
        int k = 0;
        for (int j = 0; arglist[argnum][j] != '\0'; j++) {
            if (arglist[argnum][j] != '"') arglist[argnum][k++] = arglist[argnum][j];
        }
        arglist[argnum][k] = '\0';

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
    else if (strcmp(arglist[0], "history") == 0) {
        show_history();
        return 1;
    }
    else if (strcmp(arglist[0], "set") == 0) {
        show_vars();
        return 1;
    }
    else if (strcmp(arglist[0], "jobs") == 0) {
                show_jobs();
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


// Feature 8: shell variables stored in a linked list
struct var {
    char* name;
    char* value;
    struct var* next;
};
struct var* var_list = NULL;     // head of the list

// Is this line like NAME=value ? (name: letters, digits, _ ; no spaces before =)
int is_assignment(char* line) {
    while (*line == ' ' || *line == '\t') line++;
    if (!(isalpha(*line) || *line == '_')) return 0;
    while (isalnum(*line) || *line == '_') line++;
    return *line == '=';
}

// Save NAME=value in the list (update it if NAME already exists)
void do_assignment(char* line) {
    while (*line == ' ' || *line == '\t') line++;
    char* eq = strchr(line, '=');
    char* name = strndup(line, eq - line);

    // value = everything after =, without double quotes and trailing spaces
    char* value = malloc(strlen(eq + 1) + 1);
    int k = 0;
    for (char* p = eq + 1; *p != '\0'; p++) {
        if (*p != '"') value[k++] = *p;
    }
    while (k > 0 && (value[k - 1] == ' ' || value[k - 1] == '\t')) k--;
    value[k] = '\0';

    for (struct var* v = var_list; v != NULL; v = v->next) {
        if (strcmp(v->name, name) == 0) {     // already exists: replace value
            free(v->value);
            v->value = value;
            free(name);
            return;
        }
    }
    struct var* v = malloc(sizeof(struct var)); // new variable: add at front
    v->name = name;
    v->value = value;
    v->next = var_list;
    var_list = v;
}

char* get_var(char* name) {
    for (struct var* v = var_list; v != NULL; v = v->next) {
        if (strcmp(v->name, name) == 0) return v->value;
    }
    return NULL;
}

void show_vars() {
    for (struct var* v = var_list; v != NULL; v = v->next) {
        printf("%s=%s\n", v->name, v->value);
    }
}

// Replace every argument that starts with $ by the variable's value
void expand_variables(char** arglist) {
    for (int i = 0; arglist[i] != NULL; i++) {
        if (arglist[i][0] == '$' && arglist[i][1] != '\0') {
            char* value = get_var(arglist[i] + 1);   // skip the $
            if (value == NULL) value = "";            // unknown variable = empty
            free(arglist[i]);                         // free the old "$NAME"
            arglist[i] = strdup(value);               // new memory for the value
        }
    }
}
