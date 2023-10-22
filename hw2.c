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

void reap_the_child(int sig);

int is_child_terminated(pid_t child_pid);

void add_to_set(int element);

int is_in_set(int element);

void remove_from_set(int element);

void kill_and_reap_all_running_children();


int ch_PID = -1, num_children = 0;
pid_t child_pids[10];

// typedef struct 
// {
//     int job_id;
//     pid_t pid;
//     int state;
// } Job;



int main()
{
    // If SIG_INT is detected, then any foreground running processes will get terminated. Otherwise, nothing will happen.
    signal(SIGINT, handle_sigint);

    // When a child terminates, a signal will be sent to the parent to reap it
    signal(SIGCHLD, reap_the_child);

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
                setpgid(pid, 0);
                execv(splitVals[0], splitVals);

                // Only true if unable to execute executable file
                printf("Invalid Request.\n");
                exit(1);
            } 
            else if (pid > 0) // Parent process
            {
                add_to_set(pid);

                
                
                // Foreground Process
                if (splitVals[1] == NULL || strcmp(splitVals[1], "&") != 0)
                {
                    ch_PID = pid;

                    int status;
                    waitpid(pid, &status, 0);
                    remove_from_set(ch_PID);
                }
                ch_PID = -1;
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

    // They shall have no mercy
    kill_and_reap_all_running_children();

}

char* getNextUserInput()
{
    printf("%d\n", num_children);
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
    int status;

    if (is_in_set(ch_PID))
    {
        kill(ch_PID, SIGINT);
        waitpid(ch_PID, &status, 0);
        remove_from_set(ch_PID);
    }
}

void reap_the_child(int sig) 
{
    int status;

    pid_t pids[10];
    int size = 0;

    // Reap all terminated child processes (Running child processes are safe)
    for (int i = 0; i < num_children; i++)
    {
        if (is_child_terminated(child_pids[i]) == 1)
        {
            pids[size] = child_pids[i];
            size++;
        }
    }

    for (int i = 0; i < size; i++)
    {
        waitpid(pids[i], &status, 0);
        remove_from_set(pids[i]);
    }
}

int is_child_terminated(pid_t child_pid) 
{
    int status;
    pid_t result = waitpid(child_pid, &status, WNOHANG);

    if (result == 0)
        return 0;
    else if (result == child_pid) // Child process has terminated
        return 1;
    else 
    {
        // An error occurred
        return -1;
    }
}

void kill_and_reap_all_running_children()
{
    int status;

    for (int i = 0; i < num_children; i++)
    {
        kill(child_pids[i], SIGINT);
        waitpid(child_pids[i], &status, 0);
    }

    num_children = 0;
}






// Set functions for child pid's array
// ******************************************************
void add_to_set(int element) 
{
    // Check if the element is already in the set
    for (int i = 0; i < num_children; i++) 
    {
        if (child_pids[i] == element)
            return;
    }

    // Add the element to the set
    child_pids[num_children] = element;
    num_children++;
}

int is_in_set(int element) 
{
    for (int i = 0; i < num_children; i++) 
    {
        if (child_pids[i] == element)
            return 1;
    }
    return 0;
}

void remove_from_set(int element) 
{
    for (int i = 0; i < num_children; i++) 
    {
        if (child_pids[i] == element) 
        {
            // Shift all elements to fill the gap
            for (int j = i; j < num_children - 1; j++) 
                child_pids[j] = child_pids[j + 1];

            num_children--;
            return;
        }
    }
}

// ******************************************************
