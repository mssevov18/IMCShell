#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

char* major = "4";
char* minor = "0";
char* author = "mssevov, bbkanev";

char*
get_cwd()
{
    // Dynamically allocate memory
    char* cwd = (char*)malloc(PATH_MAX * sizeof(char));
    if (cwd == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    if (getcwd(cwd, PATH_MAX) == NULL)
    {
        perror("getcwd() error");
        free(cwd);  // Free memory in case of an error
        return NULL;
    }
    return cwd;  // Return dynamically allocated memory
}

// Helper function to execute a command with arguments
void
execute_command(char* command, char** args, bool show_finished)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("Fork failed");
        return;
    }
    else if (pid == 0)  // Child process
    {
        execvp(command, args);

        // If execvp fails, print an error and exit child process
        perror("exec failed");
        exit(1);
    }
    else  // Parent process
    {
        int status;
        waitpid(pid, &status, 0);  // Wait for child process to finish
        if (show_finished)
            printf("\nChild process with PID %d finished.\n", pid);
    }
}

int
main(int argc, char* argv[])
{
#ifdef _WIN32
    printf("This shell runs only on linux.\n");
    return 1;
#endif

    char* input =
        (char*)malloc(256 * sizeof(char));  // Dynamically allocate memory
    if (input == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    int status_code = 1;  // 0: exit, 1: normal running
    char* command;
    char* cwd;
    char* user = getenv("USER");

    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);

    while (status_code == 1)
    {
        cwd = get_cwd();
        if (cwd != NULL)
        {
            printf("%s@%s~%s> ", user, hostname, cwd);
            free(cwd);  // Free memory allocated by get_cwd
            cwd = NULL;
        }
        else
            printf("%s@%s> ", user, hostname);  // Handle case where cwd is NULL

        if (fgets(input, 256, stdin) == NULL)  // Safely read input
            continue;

        // Remove newline character if present
        input[strcspn(input, "\n")] = '\0';
        command = strtok(input, " ");

        // Ignore empty inputs
        if (command == NULL) continue;

        // Stop all foreground and background processes;
        if (!strcmp(command, "exit"))
        {
            // TODO: Should check for active sub-processes
            // If there are any, ask:
            // Wait for background processes? Y/n
            status_code = 0;
            break;
        }
        // Display all implemented commands
        // TODO add help <command> that would display command specific help
        else if (!strcmp(command, "help"))
            printf(
                "This is help "
                "command\nCommands:\n\texit\n\thelp\n\tglobalusage\n\n");
        // Current version, authors
        else if (!strcmp(command, "globalusage"))
            printf("IMCSH Version %s.%s created by %s\n", major, minor, author);
        // Exec implementation
        else if (!strcmp(command, "exec"))
        {
            char*
                args[64];  // Array to store arguments, max 63 + NULL terminator
            int i = 0;

            // Get the first argument (the command to execute)
            args[i++] = strtok(NULL, " ");

            if (args[0] == NULL)
            {
                printf("Usage: exec <command> [arguments]\n");
                continue;
            }

            // Get the rest of the arguments
            char* token = strtok(NULL, " ");
            while (token != NULL && i < 63)
            {
                args[i++] = token;
                token = strtok(NULL, " ");
            }
            args[i] = NULL;  // Null-terminate the argument list

            // Call the helper function
            execute_command(args[0], args, true);
        }
        // Debug, for now
        else
        {
            printf("%s ", command);
            char* token;
            token = strtok(NULL, " ");  // First token
            while (token != NULL)
            {
                printf("%s ", token);  // Process the token (e.g., print it)
                token = strtok(NULL, " ");  // Get the next token
            }
            printf("\n");
        }
    }

    free(input);  // Free dynamically allocated memory
    return 0;
}
