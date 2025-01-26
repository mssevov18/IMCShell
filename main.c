#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* major = "0";
char* minor = "1";
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

int
main(int argc, char* argv[])
{
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

        if (command == NULL) continue;

        if (strcmp(command, "exit") == 0)
        {
            // TODO: Should check for active sub-processes
            status_code = 0;
            break;
        }
        else if (strcmp(command, "help") == 0)
            printf(
                "This is help "
                "command\nCommands:\n\texit\n\thelp\n\tglobalusage\n\n");
        else if (strcmp(command, "globalusage") == 0)
            printf("IMCSH Version %s.%s created by %s\n", major, minor, author);
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
