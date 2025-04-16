/**
 * UNIX Shell Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_TOKENS 100
#define MAX_HISTORY 100

//variables(global)
char history[MAX_HISTORY][MAX_COMMAND_LENGTH];
int history_count = 0;

// Functions
void display_prompt();
void read_command(char *command);
void parse_command(char *command, char *tokens[], int *token_count);
void execute_command(char *tokens[], int token_count);
void handle_redirection(char *tokens[], int *token_count);
void handle_piping(char *tokens[], int token_count);
void execute_multiple_commands(char *command);
void execute_logical_commands(char *command);
void add_to_history(char *command);
void display_history();
void handle_signal(int sig);

int main() {
    char command[MAX_COMMAND_LENGTH];
    char *tokens[MAX_TOKENS];
    int token_count;
    
    // Set up signal handling for CTRL+C
    signal(SIGINT, handle_signal);
    
    while (1) {
        display_prompt();
        read_command(command);
        
        // Exit command
        if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            break;
        }
        
        // Add command to history if it's not empty
        if (strlen(command) > 0) {
            add_to_history(command);
        }
        
        // Check for multiple commands separated by semicolons
        if (strchr(command, ';') != NULL) {
            execute_multiple_commands(command);
            continue;
        }
        
        // Check for logical AND operator
        if (strstr(command, "&&") != NULL) {
            execute_logical_commands(command);
            continue;
        }
        
        // Handle history command
        if (strcmp(command, "history") == 0) {
            display_history();
            continue;
        }
        
        // Parse the command into tokens
        parse_command(command, tokens, &token_count);
        
        // Execute the command
        if (token_count > 0) {
            execute_command(tokens, token_count);
        }
    }
    
    return 0;
}

// Display the shell prompt

void display_prompt() {
    printf("sh> ");
    fflush(stdout);
}

// Read the command from the user
 
void read_command(char *command) {
    fgets(command, MAX_COMMAND_LENGTH, stdin);
    
    // Remove trailing newline
    size_t len = strlen(command);
    if (len > 0 && command[len - 1] == '\n') {
        command[len - 1] = '\0';
    }
}


// Parse the command into tokens

void parse_command(char *command, char *tokens[], int *token_count) {
    char *token = strtok(command, " \t");
    *token_count = 0;
    
    while (token != NULL && *token_count < MAX_TOKENS - 1) {
        tokens[(*token_count)++] = token;
        token = strtok(NULL, " \t");
    }
    
    tokens[*token_count] = NULL; // Null-terminate the tokens array
}


// Execute a single command with potential redirection and piping

void execute_command(char *tokens[], int token_count) {
    // Check for pipe
    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], "|") == 0) {
            handle_piping(tokens, token_count);
            return;
        }
    }
    
    // Save a copy of tokens for redirection
    char *tokens_copy[MAX_TOKENS];
    int tokens_copy_count = token_count;
    for (int i = 0; i < token_count; i++) {
        tokens_copy[i] = tokens[i];
    }
    tokens_copy[token_count] = NULL;
    
    // Built-in command: cd
    if (strcmp(tokens[0], "cd") == 0) {
        if (token_count == 1 || strcmp(tokens[1], "~") == 0) {
            chdir(getenv("HOME"));
        } else {
            if (chdir(tokens[1]) != 0) {
                perror("cd");
            }
        }
        return;
    }
    
    // Fork a child process to execute the command
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        
        // Handle redirection in child process only
        handle_redirection(tokens_copy, &tokens_copy_count);
        
        if (execvp(tokens_copy[0], tokens_copy) < 0) {
            printf("Command not found: %s\n", tokens_copy[0]);
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        wait(NULL);
    }
}


// Handle input/output redirection

void handle_redirection(char *tokens[], int *token_count) {
    int in_file = -1, out_file = -1;
    int i, j;
    int append = 0;
    
    // Look for redirection symbols
    for (i = 0; i < *token_count; i++) {
        // Input redirection
        if (strcmp(tokens[i], "<") == 0) {
            if (i + 1 < *token_count) {
                in_file = open(tokens[i + 1], O_RDONLY);
                if (in_file < 0) {
                    perror("open");
                    return;
                }
                // Redirect stdin
                dup2(in_file, STDIN_FILENO);
                close(in_file);
            }
            
            // Remove the redirection symbol and filename from tokens
            for (j = i; j < *token_count - 2; j++) {
                tokens[j] = tokens[j + 2];
            }
            *token_count -= 2;
            i--;
        }
        // Output redirection (append)
        else if (strcmp(tokens[i], ">>") == 0) {
            if (i + 1 < *token_count) {
                out_file = open(tokens[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
                if (out_file < 0) {
                    perror("open");
                    return;
                }
                // Redirect stdout
                dup2(out_file, STDOUT_FILENO);
                close(out_file);
            }
            
            // Remove the redirection symbol and filename from tokens
            for (j = i; j < *token_count - 2; j++) {
                tokens[j] = tokens[j + 2];
            }
            *token_count -= 2;
            i--;
        }
        // Output redirection (overwrite)
        else if (strcmp(tokens[i], ">") == 0) {
            if (i + 1 < *token_count) {
                out_file = open(tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (out_file < 0) {
                    perror("open");
                    return;
                }
                // Redirect stdout
                dup2(out_file, STDOUT_FILENO);
                close(out_file);
            }
            
            // Remove the redirection symbol and filename from tokens
            for (j = i; j < *token_count - 2; j++) {
                tokens[j] = tokens[j + 2];
            }
            *token_count -= 2;
            i--;
        }
    }
    
    tokens[*token_count] = NULL; // Ensure null-termination
}


//Handle command piping

void handle_piping(char *tokens[], int token_count) {
    int i, j, k;
    int pipe_count = 0;
    int cmd_start[MAX_TOKENS];
    int cmd_end[MAX_TOKENS];
    int pipes[MAX_TOKENS][2];
    
    // Count pipes and mark the start/end of commands
    cmd_start[0] = 0;
    for (i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], "|") == 0) {
            cmd_end[pipe_count] = i - 1;
            pipe_count++;
            cmd_start[pipe_count] = i + 1;
            tokens[i] = NULL; // Replace pipe symbol with NULL for execvp
        }
    }
    cmd_end[pipe_count] = token_count - 1;
    
    // Create pipes
    for (i = 0; i < pipe_count; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return;
        }
    }
    
    // Execute commands with piping
    for (i = 0; i <= pipe_count; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            
            // Set up input redirection from previous pipe (except for first command)
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            
            // Set up output redirection to next pipe (except for last command)
            if (i < pipe_count) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            // Close all pipe file descriptors
            for (j = 0; j < pipe_count; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Create a new array with just this command's tokens
            char *cmd_tokens[MAX_TOKENS];
            k = 0;
            for (j = cmd_start[i]; j <= cmd_end[i]; j++) {
                cmd_tokens[k++] = tokens[j];
            }
            cmd_tokens[k] = NULL;
            
            // Handle redirection for this command
            int token_count_cmd = k;
            handle_redirection(cmd_tokens, &token_count_cmd);
            
            // Execute the command
            if (execvp(cmd_tokens[0], cmd_tokens) < 0) {
                printf("Command not found: %s\n", cmd_tokens[0]);
                exit(EXIT_FAILURE);
            }
        }
    }
    
    // Parent process: close all pipe file descriptors
    for (i = 0; i < pipe_count; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for all child processes to finish
    for (i = 0; i <= pipe_count; i++) {
        wait(NULL);
    }
}


// Execute multiple commands separated by semicolons

void execute_multiple_commands(char *command) {
    char command_copy[MAX_COMMAND_LENGTH];
    strcpy(command_copy, command);  // Create a copy to preserve the original command
    
    char *cmd = strtok(command_copy, ";");
    
    while (cmd != NULL) {
        // Skip leading whitespace
        while (*cmd == ' ' || *cmd == '\t') {
            cmd++;
        }
        
        // Parse and execute this command
        char *tokens[MAX_TOKENS];
        int token_count;
        
        // Create a temporary copy of this command for parsing
        char cmd_copy[MAX_COMMAND_LENGTH];
        strcpy(cmd_copy, cmd);
        
        parse_command(cmd_copy, tokens, &token_count);
        
        if (token_count > 0) {
            execute_command(tokens, token_count);
        }
        
        // Get the next command
        cmd = strtok(NULL, ";");
    }
}

/**
 * Execute logical commands separated by &&
 */
void execute_logical_commands(char *command) {
    char *cmd = strtok(command, "&");
    char next_cmd[MAX_COMMAND_LENGTH];
    
    // Get the first command
    strcpy(next_cmd, cmd);
    
    while (cmd != NULL) {
        // Get the next command (skip the second '&')
        cmd = strtok(NULL, "&");
        
        if (cmd != NULL) {
            // Skip leading whitespace
            while (*cmd == ' ' || *cmd == '\t') {
                cmd++;
            }
            
            // Execute the current command
            char *tokens[MAX_TOKENS];
            int token_count;
            
            parse_command(next_cmd, tokens, &token_count);
            
            if (token_count > 0) {
                pid_t pid = fork();
                
                if (pid < 0) {
                    perror("fork");
                    exit(EXIT_FAILURE);
                } else if (pid == 0) {
                    // Child process
                    if (execvp(tokens[0], tokens) < 0) {
                        printf("Command not found: %s\n", tokens[0]);
                        exit(EXIT_FAILURE);
                    }
                } else {
                    // Parent process
                    int status;
                    waitpid(pid, &status, 0);
                    
                    // If the command failed, don't execute the next one
                    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                        return;
                    }
                }
            }
            
            // Save the next command
            strcpy(next_cmd, cmd);
        } else {
            // Execute the last command
            char *tokens[MAX_TOKENS];
            int token_count;
            
            parse_command(next_cmd, tokens, &token_count);
            
            if (token_count > 0) {
                execute_command(tokens, token_count);
            }
        }
    }
}


//  Add a command to the history

void add_to_history(char *command) {
    if (history_count < MAX_HISTORY) {
        strcpy(history[history_count++], command);
    } else {
        // Shift the history array to make room for the new command
        for (int i = 1; i < MAX_HISTORY; i++) {
            strcpy(history[i-1], history[i]);
        }
        strcpy(history[MAX_HISTORY-1], command);
    }
}


// Display the command history

void display_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d: %s\n", i+1, history[i]);
    }
}

// Handle signals such as CTRL+C

void handle_signal(int sig) {
    if (sig == SIGINT) {
        printf("\n");
        display_prompt();
        fflush(stdout);
    }
}