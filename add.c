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

int main(int argc, char* argv[])
{
    int n = atoi(argv[1]);

    printf("%d \n", n + 2); // print n + 2

    return 0;
}