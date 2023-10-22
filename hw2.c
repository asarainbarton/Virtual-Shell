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
#include <stdbool.h>


void checkValidPtr(char* ptr);

char* getEmptyString();

char* readString();

char** split(char* inputStr);

char* getNextUserInput();

void handle_sigint(int sig);

void reap_the_child(int sig);

void handle_ctrl_z(int sig);

int is_child_terminated(pid_t child_pid);

void add_to_set(int element);

int is_in_set(int element);

void remove_from_set(int element);

void kill_and_reap_all_running_children();

void update_jobs(int pid, int status, char** command);

char** deepCopy(char** original);

char* combineStrings(char** strings);

bool jobExists(int pid);



// Fun Global Stuff!!

int ch_PID = -1, num_children = 0;
pid_t child_pids[5];

typedef struct 
{
    int job_id;
    pid_t pid;
    int state;
    char** command;
} Job;

Job jobs[5];

int num_jobs = 0;

char** G_splitVals;



int main()
{
    // If SIG_INT is detected, then any foreground running processes will get terminated. Otherwise, nothing will happen.
    signal(SIGINT, handle_sigint);

    // When a child terminates, a signal will be sent to the parent to reap it
    signal(SIGCHLD, reap_the_child);

    // Handles ctrl + z (stopping only the child)
    signal(SIGTSTP, handle_ctrl_z);

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

        G_splitVals = splitVals;

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
        else if (strcmp(splitVals[0], "jobs") == 0)
        {
            for (int i = 0; i < num_jobs; i++)
            {
                char s[12];
                if (jobs[i].state == 0)
                    strcpy(s, "Running");
                else if (jobs[i].state == 1)
                    strcpy(s, "Running");
                else 
                    strcpy(s, "Stopped");

                char* cmd = combineStrings(jobs[i].command);

                printf("[%d] (%d) %s %s\n", i + 1, jobs[i].pid, s, cmd);
            }
        }
        else if (strcmp(splitVals[0], "fg") == 0 || strcmp(splitVals[0], "bg") == 0 || strcmp(splitVals[0], "kill") == 0)
        {
            if (splitVals[1] != NULL)
            {
                int the_pid = -1;
                // User enterred by job id
                if (splitVals[1][0] == '%') 
                {
                    char* newS = splitVals[1] + 1;
                    
                    // Must be in correct range
                    if (atoi(newS) >= 1 && atoi(newS) <= num_jobs)
                        the_pid = jobs[atoi(newS) - 1].pid;
                }
                else // User enterred by pid
                {
                    if (is_in_set(atoi(splitVals[1])))
                        the_pid = atoi(splitVals[1]);
                }

                // If true, then we can resume whatever process was stopped or running in the background
                if (the_pid != -1)
                {
                    if (strcmp(splitVals[0], "fg") == 0)
                    {
                        ch_PID = the_pid;
                        kill(the_pid, SIGCONT);

                        int status;
                        waitpid(the_pid, &status, WUNTRACED);

                        if (WIFEXITED(status))
                            remove_from_set(ch_PID);
                        else if (is_in_set(the_pid)) // The process was in the foreground but then got stopped again
                        {
                            char sl[6] = "sleep";
                            char** G = malloc(2*sizeof(char*));
                            G[0] = sl;
                            G[1] = NULL;
                            update_jobs(the_pid, 2, deepCopy(G));
                        }
                            
                    }
                    // When we want a stopped process to be a background process
                    else if (strcmp(splitVals[0], "bg") == 0)
                    {
                        kill(the_pid, SIGCONT);
                        if (is_in_set(the_pid))
                            update_jobs(the_pid, 0, deepCopy(splitVals));
                    }
                    else // Kill command enterred
                    {
                        int stat;
                        // If stopped, we want to make it a background process first before terminating it since linux doesn't seem to
                        // like us terminating stopped jobs.
                        kill(the_pid, SIGCONT);  
                        kill(the_pid, SIGINT);
                        waitpid(the_pid, &stat, WUNTRACED);
                        remove_from_set(the_pid);
                    }
                        
                }
                else 
                    printf("Invalid Request.\n");

                ch_PID = -1;
            }
            else 
                printf("Invalid Request.\n");
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
                    waitpid(pid, &status, WUNTRACED);

                    if (WIFEXITED(status))
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
    // printf("%d\n", num_children);
    printf("prompt >");
    char* input = readString();

    return input;
}

char** split(char* inputStr)
{
    char** splitVals = NULL;
    char* str = malloc(sizeof(char));
    str[0] = '\0';

    int count = 0, bigCount = 0;
    
    for (int i = 0; i < strlen(inputStr); i++)
    {
        if (inputStr[i] != ' ' && inputStr[i] != 9)
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


// Signal Handlers
// ***********************************************************
void handle_sigint(int sig) 
{
    int status;

    // Then we know it's a foreground job
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

    pid_t pids[5];
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

void handle_ctrl_z(int sig)
{
    if (is_in_set(ch_PID))
    {
        kill(ch_PID, SIGTSTP);

        char sl[6] = "sleep";
        char** G = malloc(2*sizeof(char*));
        G[0] = sl;
        G[1] = NULL;

        update_jobs(ch_PID, 2, deepCopy(G));
    }
        
    
}

// ***********************************************************

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

bool jobExists(int pid) 
{
    for (int i = 0; i < num_jobs; i++) 
    {
        if (jobs[i].pid == pid) {
            return true;
        }
    }
    return false;
}

void update_jobs(int pid, int status, char** command)
{
    // Then job status most-likely changed
    if (is_in_set(pid) && jobExists(pid))
    {
        int index;
        for (int i = 0; i < num_jobs; i++)
        {
            if (jobs[i].pid == pid)
            {
                index = i;
                break;
            }
        }

        jobs[index].state = status;
        jobs[index].command = command;
    }
    // Then the job needs to be added to the job list
    else if (is_in_set(pid) && ! jobExists(pid))
    {
        jobs[num_jobs].pid = pid;
        jobs[num_jobs].state = status;
        jobs[num_jobs].command = command;

        num_jobs++;
    }
    // Then the job needs to be removed from the job list
    else 
    {
        int index;
        for (int i = 0; i < num_jobs; i++)
        {
            if (jobs[i].pid == pid)
            {
                index = i;
                break;
            }
        }

        for (int i = index; i < num_jobs - 1; i++)
        {
            jobs[i] = jobs[i+1];
        }

        num_jobs--;
    }
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

    int stat;
    // 0 for Foreground/Running, 1 for Background/Running, 2 for Stopped
    if (G_splitVals[1] == NULL || strcmp(G_splitVals[1], "&") != 0)
    {
        stat = 0;
    }
    else
    {
        stat = 1;
    }
    char** v = deepCopy(G_splitVals);
    update_jobs(element, stat, v);

    
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
            update_jobs(element, 1, NULL);
            return;
        }
    }
}

// ******************************************************


char** deepCopy(char** original) 
{
    int i = 0;
    while (original[i] != NULL) {
        i++;
    }

    char** copy = malloc((i + 1) * sizeof(char*));
    if (copy == NULL) {
        return NULL; 
    }

    for (int j = 0; j < i; j++) {
        int len = strlen(original[j]) + 1;
        copy[j] = malloc(len * sizeof(char));
        if (copy[j] == NULL) {
            
            for (int k = 0; k < j; k++) {
                free(copy[k]);
            }
            free(copy);
            return NULL; 
        }
        strcpy(copy[j], original[j]);
    }
    copy[i] = NULL; 

    return copy;
}

char* combineStrings(char** strings) 
{
    int i = 0;
    while (strings[i] != NULL) {
        i++;
    }

    char* combined = malloc((i + 1) * sizeof(char));
    if (combined == NULL) {
        return NULL; 
    }

    for (int j = 0; j < i; j++) {
        strcat(combined, strings[j]);
        strcat(combined, " ");
    }

    return combined;
}
