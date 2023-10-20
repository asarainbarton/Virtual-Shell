// Specific Headers
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>

// Additional Headers



void checkValidPtr(char* ptr);

char* getEmptyString();

char* readString();

char** split(char* inputStr);

char* getNextUserInput();

void handle_sigint(int sig);

int ch_PID;

int main()
{

    char* input = getNextUserInput();
    char** splitVals = NULL;
    while (strcmp(input, "quit") != 0)
    {
        splitVals = split(input);

        // No command was written
        if (splitVals == NULL || *splitVals == NULL)
        {
            free(splitVals);
            input = getNextUserInput();
            continue;
        }

        // ********************************************************

        if (strcmp(splitVals[0], "cd") == 0)
        {
            char* arg;

            if (splitVals[1] == NULL)
            {
                arg = malloc(2*sizeof(char));

                // Having an empty argument will change to the home directory by default.
                arg[0] = '/';
                arg[1] = '\0';
            }
            else
                arg = splitVals[1];
            
            if (chdir(arg) != 0)
                perror("Error");
        }
        else if (strcmp(splitVals[0], "pwd") == 0)
        {
            char cwd[200];
            if (getcwd(cwd, sizeof(cwd)) != NULL)
                printf("%s\n", cwd);
            else 
                perror("Error");
        }
        else // Command may be attempting to run an executable of name splitVals[0]
        {
            pid_t pid = fork();

            if (pid == 0) // Child process
            {
                execv(splitVals[0], splitVals);

                // Only true is unable to execute executable file
                printf("Invalid Request.\n");
                exit(1);
            } 
            else if (pid > 0) // Parent process
            {
                ch_PID = pid;
                signal(SIGINT, handle_sigint);
                
                int status;
                waitpid(pid, &status, 0);

                struct sigaction act;
                memset(&act, 0, sizeof(struct sigaction));
                act.sa_flags = SA_RESETHAND;
                act.sa_handler = handle_sigint;
                sigaction(SIGINT, &act, NULL);
            } 
            else // Fork failed
                printf("Invalid Request.\n");
            
            
        }
        







        

        // ********************************************************

        // Free all memory from splitVals
        for (int i = 0; splitVals[i] != NULL; i++)
            free(splitVals[i]);
        free(splitVals);

        input = getNextUserInput();
    }

}

char* getNextUserInput()
{
    printf("prompt >");
    char* input = readString();

    return input;
}

char** split(char* inputStr)
{
    char delimiter = ' ';
    char** splitVals = NULL;
    char* str = malloc(sizeof(char));
    str[0] = '\0';

    int count = 0, bigCount = 0;
    
    for (int i = 0; i < strlen(inputStr); i++)
    {
        if (inputStr[i] != delimiter)
        {
            count++;
            str = realloc(str, (count + 1) * sizeof(char));
            str[count - 1] = inputStr[i];
            str[count] = '\0';

            if (i == strlen(inputStr) - 1)
            {
                bigCount++;
                splitVals = realloc(splitVals, bigCount * sizeof(char*));
                splitVals[bigCount - 1] = str;
            }

            continue;
        }

        // We don't want empty strings to be part of our array of strings
        if (strlen(str) == 0)
            continue;
        
        bigCount++;
        splitVals = realloc(splitVals, bigCount * sizeof(char*));
        splitVals[bigCount - 1] = str;

        str = malloc(sizeof(char));
        str[0] = '\0';
        
        count = 0;
    }

    // Add a terminating NULL value to the end of the data structure to signify that the end has been reached
    splitVals = realloc(splitVals, (bigCount + 1) * sizeof(char*));
    splitVals[bigCount] = NULL;

    return splitVals;
}

char* readString() 
{
    char* str = getEmptyString();

    int ch, str_len = 0;
    while ((ch = getchar()) != EOF) 
    {
        if (ch == '\n')
            break;
        
        str_len++;
        str = realloc(str, (str_len + 1) * sizeof(char));
        checkValidPtr(str);
        
        str[str_len - 1] = ch;
        str[str_len] = '\0';
    }

    return str;
}

char* getEmptyString()
{
    char* emptyStr = malloc(sizeof(char));
    checkValidPtr(emptyStr);

    *emptyStr = '\0';

    return emptyStr;
}

void checkValidPtr(char* ptr)
{
    if (ptr == NULL)
    {
        printf("Error: Unable to process user input");
        exit(1);
    }
}

void handle_sigint(int sig) 
{
    kill(ch_PID, SIGINT);
}



