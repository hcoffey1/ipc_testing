/**********
 * Hayden Coffey
 *
 **********/
#include "util.h"

long FILE_SIZE;

int main(int argc, char **argv)
{

    // Setup pipes------------------
    // Parent ---> Child messages
    int p1fd[2];

    // Child ---> Parent messages
    int p2fd[2];

    if (pipe(p1fd) == -1)
    {
        fprintf(stderr, "Failed to create pipe.\n");
        return 1;
    }

    if (pipe(p2fd) == -1)
    {
        fprintf(stderr, "Failed to create pipe.\n");
        return 1;
    }

    // Map sample data into memory--------
    void *data = map_file("brown.txt", &FILE_SIZE);

    // Fork child process----------------
    pid_t id;
    id = fork();

    if (id < 0)
    {
        fprintf(stderr, "Fork failed\n");
        return 1;
    }

    // Parent process--------
    else if (id == 0)
    {
        printf("Inside parent process\n");

        close(p1fd[0]);

        char inputstr[100];
        scanf("%s", inputstr);

        write(p1fd[1], inputstr, strlen(inputstr) + 1);
        close(p1fd[1]);

        wait(NULL);
    }

    // Child process---------
    else if (id > 0)
    {
        printf("Inside child process\n");

        close(p1fd[1]);

        char buf[100];

        read(p1fd[0], buf, 100);

        printf("Read the string : %s\n", buf);

        close(p1fd[0]);
    }

    return 0;
}
