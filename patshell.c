#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_INPUT 1024
#define MAX_ARGS 100

void execute_command(char **args);
int handle_redirection(char **args);
int handle_pipes(char *input);

void shell_loop() {
    char input[MAX_INPUT];

    while (1) {
        printf("PatShell$ ");
        fflush(stdout);

        if (!fgets(input, MAX_INPUT, stdin)) break;

        // Remove newline
        input[strcspn(input, "\n")] = '\0';

        // Skip empty input
        if (strlen(input) == 0) continue;

        // Handle piping if present
        if (strchr(input, '|')) {
            handle_pipes(input);
        } else {
            // Tokenize input
            char *args[MAX_ARGS];
            int i = 0;
            char *token = strtok(input, " ");
            while (token != NULL && i < MAX_ARGS - 1) {
                args[i++] = token;
                token = strtok(NULL, " ");
            }
            args[i] = NULL;

            if (args[0] == NULL) continue;

            // Built-in command: exit
            if (strcmp(args[0], "exit") == 0) break;

            // Built-in command: cd
            if (strcmp(args[0], "cd") == 0) {
                if (args[1]) {
                    chdir(args[1]);
                } else {
                    fprintf(stderr, "cd: expected argument\n");
                }
                continue;
            }

            // Handle I/O redirection
            if (handle_redirection(args)) continue;

            // External command
            execute_command(args);
        }
    }
}

void execute_command(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], args);
        perror("exec");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        wait(NULL);
    } else {
        perror("fork");
    }
}

int handle_redirection(char **args) {
    int i = 0;
    int redirected = 0;

    while (args[i]) {
        if (strcmp(args[i], ">") == 0) {
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { perror("open"); return 1; }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL;
            redirected = 1;
            break;
        } else if (strcmp(args[i], "<") == 0) {
            int fd = open(args[i + 1], O_RDONLY);
            if (fd < 0) { perror("open"); return 1; }
            dup2(fd, STDIN_FILENO);
            close(fd);
            args[i] = NULL;
            redirected = 1;
            break;
        }
        i++;
    }

    if (redirected) {
        execute_command(args);
        return 1;
    }

    return 0;
}

int handle_pipes(char *input) {
    int pipefd[2];
    pid_t pid;

    char *cmd1 = strtok(input, "|");
    char *cmd2 = strtok(NULL, "|");

    if (!cmd1 || !cmd2) {
        fprintf(stderr, "Invalid pipe syntax\n");
        return 1;
    }

    // Trim whitespace
    while (*cmd1 == ' ') cmd1++;
    while (*cmd2 == ' ') cmd2++;

    pipe(pipefd);
    pid = fork();

    if (pid == 0) {
        // Child: executes cmd1 and writes to pipe
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        char *args[MAX_ARGS];
        int i = 0;
        char *token = strtok(cmd1, " ");
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        execvp(args[0], args);
        perror("exec1");
        exit(EXIT_FAILURE);
    } else {
        // Parent: executes cmd2 and reads from pipe
        wait(NULL);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[1]);
        close(pipefd[0]);

        char *args[MAX_ARGS];
        int i = 0;
        char *token = strtok(cmd2, " ");
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        execvp(args[0], args);
        perror("exec2");
        exit(EXIT_FAILURE);
    }
}

int main() {
    shell_loop();
    return 0;
}
