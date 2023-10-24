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
    int number;
    scanf("%d", &number);

    printf("You entered %d\n", number);

    printf("%d \n", number + 2); // print number + 2

    return 0;

    return 0;
}