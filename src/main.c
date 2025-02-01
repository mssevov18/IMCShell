#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_BG_PROCESSES 128

typedef struct
{
    pid_t pid;
    char command[256];
} BackgroundProcess;

BackgroundProcess bg_processes[MAX_BG_PROCESSES];
int bg_process_count = 0;

char* major = "6";
char* minor = "2";
char* author = "mssevov, bbkanev";

const char* commands[] = {"help", "globalusage", "exec",
                          "jobs", "exit/quit",   "cd"};
int n_commands = sizeof(commands) / sizeof(commands[0]);

char*
get_cwd()
{
    char* cwd = (char*)malloc(PATH_MAX * sizeof(char));
    if (cwd == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    if (getcwd(cwd, PATH_MAX) == NULL)
    {
        perror("getcwd() error");
        free(cwd);
        return NULL;
    }
    return cwd;
}

void
execute_command(char* command, char** args, bool wait_for_completion,
                char* full_command, bool print_finish)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("Fork failed");
        return;
    }
    else if (pid == 0)
    {  // Child process
        int fd = -1;

        // Handle Output Redirection (Only in Child Process)
        for (int i = 0; args[i] != NULL; i++)
        {
            if (strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0)
            {
                bool append = strcmp(args[i], ">>") == 0;

                // Ensure a filename follows the redirection operator
                if (args[i + 1] == NULL)
                {
                    fprintf(stderr,
                            "Syntax error: expected filename after %s\n",
                            args[i]);
                    exit(1);
                }

                // Open file in appropriate mode
                fd = open(args[i + 1],
                          append ? O_WRONLY | O_CREAT | O_APPEND
                                 : O_WRONLY | O_CREAT | O_TRUNC,
                          0644);
                if (fd < 0)
                {
                    perror("Failed to open file for redirection");
                    exit(1);
                }

                // Redirect stdout to the file
                if (dup2(fd, STDOUT_FILENO) < 0)
                {
                    perror("Failed to redirect stdout");
                    exit(1);
                }
                close(fd);  // Close unnecessary file descriptor

                // Remove redirection symbols from args[]
                args[i] = NULL;
                break;
            }
        }

        // Execute the command
        execvp(command, args);

        // If execvp fails, print error
        perror("execvp failed");
        exit(1);
    }
    else
    {  // Parent process
        if (wait_for_completion)
        {
            int status;
            waitpid(pid, &status, 0);  // Wait for child process to finish
            if (print_finish)
                printf("\nChild process with PID %d finished.\n", pid);
        }
        else
        {  // Background process
            if (bg_process_count < MAX_BG_PROCESSES)
            {
                bg_processes[bg_process_count].pid = pid;
                strncpy(bg_processes[bg_process_count].command, full_command,
                        256);
                bg_process_count++;
                printf("Started background process with PID %d: %s\n", pid,
                       full_command);
            }
            else
            {
                printf(
                    "Maximum background processes reached. Cannot start a new "
                    "background process.\n");
            }
        }
    }
}

void
execute_user_command(char* command, char* full_command)
{
    char* args[64];
    int i = 0;

    args[i++] = command;
    char* token = strtok(NULL, " ");
    while (token != NULL && i < 63)
    {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;  // Ensure NULL termination

    bool run_in_background = false;

    // Check if the last argument is "&" (background execution)
    if (i > 1 && strcmp(args[i - 1], "&") == 0)
    {
        run_in_background = true;
        args[i - 1] = NULL;  // Remove "&" from arguments
    }

    execute_command(command, args, !run_in_background, full_command, false);
}

void
clean_up_background_processes()
{
    for (int i = 0; i < bg_process_count; i++)
    {
        int status;
        pid_t result = waitpid(bg_processes[i].pid, &status, WNOHANG);
        if (result > 0)
        {  // Process finished
            printf("Background process with PID %d (%s) finished.\n",
                   bg_processes[i].pid, bg_processes[i].command);
            // Remove the process from the list
            for (int j = i; j < bg_process_count - 1; j++)
                bg_processes[j] = bg_processes[j + 1];
            bg_process_count--;
            i--;  // Adjust index due to shift
        }
    }
}

int
main()
{
#ifdef _WIN32
    printf("This shell runs only on linux.\n");
    return 1;
#endif

    // Fixed total len of a single line (256 characters)
    char* input = (char*)malloc(256 * sizeof(char));
    if (input == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    int status_code = 1;
    char* command;
    char* cwd;
    char* user = getenv("USER");

    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);

    while (status_code == 1)
    {
        clean_up_background_processes();  // Clean up completed background
                                          // processes

        cwd = get_cwd();
        if (cwd != NULL)
        {
            printf("%s@%s~%s> ", user, hostname, cwd);
            free(cwd);
            cwd = NULL;
        }
        else
            printf("%s@%s> ", user, hostname);

        if (fgets(input, 256, stdin) == NULL) continue;

        input[strcspn(input, "\n")] = '\0';
        command = strtok(input, " ");

        if (command == NULL)
            continue;

        else if (!strcmp(command, "exit") || !strcmp(command, "quit"))
        {
            // Check for running background processes
            if (bg_process_count > 0)
            {
                printf("The following processes are running:\n");

                // List active background processes
                for (int i = 0; i < bg_process_count; i++)
                {
                    printf("[%d] PID: %d Command: %s\n", i + 1,
                           bg_processes[i].pid, bg_processes[i].command);
                }

                // Prompt the user for confirmation
                printf("Are you sure you want to quit? [Y/n]: ");
                char choice;
                scanf(" %c", &choice);  // Get the user's input

                if (choice == 'Y' || choice == 'y')
                {
                    // Terminate all background processes
                    for (int i = 0; i < bg_process_count; i++)
                    {
                        kill(bg_processes[i].pid, SIGKILL);
                        printf(
                            "Terminated background process with PID %d (%s).\n",
                            bg_processes[i].pid, bg_processes[i].command);
                    }
                    bg_process_count = 0;  // Clear the list
                    status_code = 0;       // Exit the shell
                    break;
                }
                else if (choice == 'N' || choice == 'n')
                {
                    printf("Returning to the shell...\n");
                    continue;  // Return to the shell without exiting
                }
                else
                {
                    printf("Invalid input. Returning to the shell...\n");
                    continue;  // Invalid input, return to the shell
                }
            }
            else
            {
                // No background processes, exit immediately
                status_code = 0;
                break;
            }
        }
        else if (!strcmp(command, "help"))
        {
            printf("Commands:\n");
            for (int i = 0; i < n_commands; i++) printf("%s\n", commands[i]);
        }
        else if (!strcmp(command, "globalusage"))
            printf("IMCSH Version %s.%s created by %s\n", major, minor, author);
        else if (!strcmp(command, "jobs"))
        {
            // List background processes
            for (int i = 0; i < bg_process_count; i++)
            {
                printf("[%d] PID: %d Command: %s\n", i + 1, bg_processes[i].pid,
                       bg_processes[i].command);
            }
        }
        else if (!strcmp(command, "cd"))
        {
            char* path = strtok(NULL, " ");  // Get the target directory

            // If no path is provided, default to the user's home directory
            if (path == NULL)
            {
                path = getenv("HOME");
                if (path == NULL)
                {
                    fprintf(stderr, "cd: HOME not set\n");
                    continue;
                }
            }

            // Change directory
            if (chdir(path) != 0) perror("cd");
        }
        else if (!strcmp(command, "exec"))
        {
            char* program = strtok(NULL, " ");

            if (program == NULL)
            {
                printf("Usage: exec <command> [arguments]\n");
                continue;
            }

            execute_user_command(program, input);
        }

        // General command execution, including `exec` and output redirection
        else
            execute_user_command(command, input);
    }

    free(input);
    return 0;
}
