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
char* minor = "0";
char* author = "mssevov, bbkanev";

/* char** commands = ["help", "globalusage", "exec", "jobs", "exit/quit"]; */
/* size_t ui_commands */

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
    {                 // Child process
        int fd = -1;  // File descriptor for redirection
        for (int i = 0; args[i] != NULL; i++)
        {
            if (strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0)
            {
                bool append = strcmp(args[i], ">>") == 0;

                // Ensure the filename is specified
                if (args[i + 1] == NULL)
                {
                    fprintf(stderr,
                            "Syntax error: expected filename after %s\n",
                            args[i]);
                    exit(1);
                }

                // Open the file for redirection
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
                dup2(fd, STDOUT_FILENO);
                close(fd);

                // Remove redirection tokens from arguments
                args[i] = NULL;
                break;
            }
        }
        execvp(command, args);

        // If execvp fails, print an error and exit child process
        perror("exec failed");
        perror(full_command);
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
                printf(
                    "Maximum background processes reached. Cannot start a new "
                    "background process.\n");
        }
    }
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
main(int argc, char* argv[])
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

        if (command == NULL) continue;

        if (!strcmp(command, "exit") || !strcmp(command, "quit"))
        {
            clean_up_background_processes();  // Check for completed processes

            if (bg_process_count > 0)
            {  // If there are active background processes
                printf("There are %d background processes running.\n",
                       bg_process_count);
                char choice;
                printf("Wait for background processes? (y/n): ");
                scanf(" %c", &choice);  // Get user input

                if (choice == 'y' || choice == 'Y')
                {
                    // Wait for all background processes to finish
                    for (int i = 0; i < bg_process_count; i++)
                    {
                        int status;
                        waitpid(bg_processes[i].pid, &status,
                                0);  // Wait for process to finish
                        printf(
                            "Background process with PID %d (%s) finished.\n",
                            bg_processes[i].pid, bg_processes[i].command);
                    }
                    bg_process_count = 0;  // All processes are finished
                }
                else if (choice == 'n' || choice == 'N')
                {
                    // Terminate all background processes
                    for (int i = 0; i < bg_process_count; i++)
                    {
                        kill(bg_processes[i].pid,
                             SIGKILL);  // Forcefully terminate process
                        printf(
                            "Terminated background process with PID %d (%s).\n",
                            bg_processes[i].pid, bg_processes[i].command);
                    }
                    bg_process_count = 0;  // All processes are terminated
                }
                else
                    printf(
                        "Invalid choice. Exiting without terminating "
                        "background processes.\n");
            }

            status_code = 0;  // Exit the shell
            break;
        }
        else if (!strcmp(command, "help"))
        {
            printf("Commands:\n");
            // TODO make it with list & loop (:
            //\texit\n\thelp\n\tglobalusage\n\tjobs\n\n");
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

        else
        {  // General command execution, including `exec` and output redirection
            char* args[64];
            int i = 0;

            args[i++] = command;
            char* token = strtok(NULL, " ");
            while (token != NULL && i < 63)
            {
                args[i++] = token;
                token = strtok(NULL, " ");
            }
            args[i] = NULL;

            bool run_in_background = false;

            // Check if the last argument is "&"
            if (i > 1 && strcmp(args[i - 1], "&") == 0)
            {
                run_in_background = true;
                args[i - 1] = NULL;  // Remove "&" from arguments
            }

            bool print_pid = strcmp(args[0], "exec") == 0;

            execute_command(command, args, !run_in_background, input,
                            print_pid);
        }
    }

    free(input);
    return 0;
}
