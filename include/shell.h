#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "FCIT> "

// Function prototypes
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
int execute(char** arglist, int background);
int handle_builtin(char** arglist);

// Feature 3: history
#define HISTORY_SIZE 20
void add_to_history(char* cmd);
void show_history();
char* get_history(int n);

// Feature 6: background jobs
#define MAX_JOBS 20
void add_job(int pid, char** arglist);
void reap_jobs();
void show_jobs();

// Feature 8: shell variables
int is_assignment(char* line);
void do_assignment(char* line);
char* get_var(char* name);
void show_vars();
void expand_variables(char** arglist);

#endif // SHELL_H
