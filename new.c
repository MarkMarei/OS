#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

#define MAX_WORDS 100   // Maximum number of words in a command
#define MAX_LENGTH 1000 // Maximum input length
#define MAX_EXPORTS 100 // Maximum number of exported variables

// Global storage for exported variables
char names[MAX_EXPORTS][MAX_LENGTH]; 
char values[MAX_EXPORTS][MAX_LENGTH]; 
int export_count = 0; // Track number of stored variables

// Remove surrounding double quotes
void remove_quotes(char *str) {
    size_t len = strlen(str);
    if (len >= 2 && str[0] == '"' && str[len - 1] == '"') {
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    }
}

// Split VAR=VALUE assignments correctly
void split_assignment(const char *input, char *var_name, char *var_value) {
    char *equal_sign = strchr(input, '=');
    if (equal_sign == NULL) {
        printf("Invalid input! Expected format: VAR=VALUE\n");
        return;
    }

    size_t name_length = equal_sign - input;
    strncpy(var_name, input, name_length);
    var_name[name_length] = '\0';

    char *value_start = equal_sign + 1;
    strcpy(var_value, value_start);
    remove_quotes(var_value); // Remove surrounding quotes if present
}

// Store an exported variable
void export_variable(char *input) {
    char var_name[MAX_LENGTH];
    char var_value[MAX_LENGTH];

    split_assignment(input, var_name, var_value);

    if (strlen(var_name) == 0 || strlen(var_value) == 0) {
        printf("Invalid export format! Use: export VAR=VALUE\n");
        return;
    }

    // Update if variable already exists
    for (int i = 0; i < export_count; i++) {
        if (strcmp(names[i], var_name) == 0) {
            strcpy(values[i], var_value);
            return;
        }
    }

    // Add new variable
    if (export_count < MAX_EXPORTS) {
        strcpy(names[export_count], var_name);
        strcpy(values[export_count], var_value);
        export_count++;
    } else {
        printf("Export limit reached!\n");
    }
}

// Get variable value by name
char *get_variable_value(char *var_name) {
    for (int i = 0; i < export_count; i++) {
        if (strcmp(names[i], var_name) == 0) {
            return values[i];
        }
    }
    return NULL;
}

// Replace variables ($VAR -> value)
void replace_variables_in_args(char *args[]) {
    for (int i = 0; args[i] != NULL; i++) {
        if (args[i][0] == '$') {
            char var_name[MAX_LENGTH];
            strcpy(var_name, args[i] + 1); // Remove '$'

            char *var_value = get_variable_value(var_name);
            if (var_value) {
                // Free old argument
                free(args[i]);

                // If the variable contains spaces, split into multiple arguments
                char *new_arg = strdup(var_value);
                char *token = strtok(new_arg, " ");
                int j = i;

                while (token != NULL) {
                    args[j] = strdup(token);
                    token = strtok(NULL, " ");
                    j++;
                }

                args[j] = NULL; // Properly terminate argument list
                free(new_arg);
            }
        }
    }
}

// Handle variable expansion inside echo
void handle_echo(char *args[]) {
    char output[MAX_LENGTH] = "";
    int inside_quotes = 0;

    for (int i = 1; args[i] != NULL; i++) {
        char word[MAX_LENGTH];
        strcpy(word, args[i]);

        if (word[0] == '"') {
            inside_quotes = 1;
            memmove(word, word + 1, strlen(word)); // Remove first quote
        }
        if (word[strlen(word) - 1] == '"' && inside_quotes) {
            inside_quotes = 0;
            word[strlen(word) - 1] = '\0'; // Remove last quote
        }

        if (word[0] == '$') { // Variable replacement inside quotes
            char *var_value = get_variable_value(word + 1);
            if (var_value) {
                strcat(output, var_value);
            } else {
                strcat(output, word); // Keep original if not found
            }
        } else {
            strcat(output, word);
        }

        strcat(output, " ");
    }

    printf("%s\n", output);
}


int main() {
    const char *home = getenv("HOME");
    char input[MAX_LENGTH];
    char *args[MAX_WORDS]; 
    char cwd[MAX_LENGTH];
    chdir(home);

    while (1) {
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("mark@shell:%s$ ", cwd);
        } else {
            perror("getcwd");
            continue;
        }

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            continue;
        }

        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "exit") == 0) {
            memset(names, 0, sizeof(names));
            memset(values, 0, sizeof(values));
            export_count = 0;
            exit(0);
        }
        else if (strcmp(input, "clear") == 0) {
            system("clear"); //clear terminal
            continue;
        }

        int i = 0;
        char *token = strtok(input, " ");
        while (token != NULL) {
            args[i++] = strdup(token);
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (strcmp(args[0], "cd") == 0) {
            if (args[1] != NULL) {
                replace_variables_in_args(&args[1]); // Replace $VAR
            }
        
            if ((args[1] == NULL) || (strcmp(args[1], "~") == 0)) {
                chdir(home);
            } else {
                if (chdir(args[1]) != 0) {
                    perror("cd");
                }
            }
            continue;
        }

        else if (strcmp(args[0], "echo") == 0) {
            handle_echo(args);
            continue;
        }

        else if (strcmp(args[0], "export") == 0) {
            if (args[1] == NULL) {
                printf("Usage: export VAR=VALUE\n");
                continue;
            }

            char full_assignment[MAX_LENGTH] = "";
            for (int j = 1; args[j] != NULL; j++) {
                strcat(full_assignment, args[j]);
                if (args[j + 1] != NULL) strcat(full_assignment, " ");
            }
            export_variable(full_assignment);
            continue;
        }

        replace_variables_in_args(args); // Replace $VAR in command

        int pid = fork();
        if (pid == 0) {
            execvp(args[0], args);
            perror("execvp");
            exit(1);
        }else if (pid > 0) {
            waitpid(pid, NULL, 0);
        }

        // Free dynamically allocated memory
        for (int j = 0; args[j] != NULL; j++) {
            free(args[j]);
        }
    }

    return 0;
}