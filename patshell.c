#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>

#define INPUT_BUFFER_SIZE 1024
#define MAX_ARGS 64
#define DELIM " \t\r\n\a"

void print_prompt()
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("%s$ ", cwd);
    }
    else
    {
        perror("getcwd");
    }
}

void parse_input(char *input, char **args)
{
    int i = 0;
    char *token = strtok(input, DELIM);
    while (token != NULL && i < MAX_ARGS - 1)
    {
        args[i++] = token;
        token = strtok(NULL, DELIM);
    }
    args[i] = NULL;
}

void execute_command(char **args)
{
    pid_t pid = fork();

    if (pid == 0)
    {
        // Child process
        if (execvp(args[0], args) == -1)
        {
            perror("myshell");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        // Error forking
        perror("myshell");
    }
    else
    {
        // Parent process
        wait(NULL);
    }
}

int main()
{
    char input[INPUT_BUFFER_SIZE];
    char *args[MAX_ARGS];

    while (1)
    {
        print_prompt();

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            printf("\n");
            break;
        }

        // Remove newline character
        input[strcspn(input, "\n")] = 0;

        // Ignore empty input
        if (strlen(input) == 0)
        {
            continue;
        }

        parse_input(input, args);

        if (strcmp(args[0], "exit") == 0)
        {
            break;
        }

        execute_command(args);
    }

    return 0;
}
