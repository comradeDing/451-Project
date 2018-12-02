#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/sem.h>
#include <sys/types.h>

int testargs(int);
void unlock(int, int, int);
void lock(int, int, int);

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
} args;

int main(int argc, char** argv)
{

    if(testargs(argc))
        exit(1);

    // Get semaphore id
    key_t semkey = atoi(argv[2]);
    int semid;
    if((semid = semget(semkey, 0, 0)) == -1)
    {
        perror("semget");
        exit(1);
    }

    // process file loop
        // read one word
        // write
    
    for(int i = 1; i <= 5; i++)
    {
        unlock(semid, 0, 1);
        std::cout << "[prog2] " << i << std::endl;
        sleep(1);
        lock(semid, 0, 0);
    }

    for(int i = 1; i <= 5; i++)
    {
        unlock(semid, 1, 0);
        std::cout << "[prog2] " << i << std::endl;
        sleep(1);
        lock(semid, 1, 1);
    }

    exit(0);
}

/**
 * FUNCTION: testargs
 * -------------------------
 * Tests if there are the correct number of command line arguments
 */ 
int testargs(int argc)
{
    if(argc != 4)
    {
        std::cout << "Usage: ./PROG2 <read pipe 1 id> <write pipe 2 id> <semaphore key> <shared mem key>" << std::endl;
        return 1;
    }
    return 0;
}

void unlock(int semid, int semnum, int semval)
{
    int val;
    while((val = semctl(semid, semnum, GETVAL, args)) != semval);
    if(val == -1)
    {
        perror("semctl");
    }
}

void lock(int semid, int semnum, int semval)
{
    args.val = semval;
    if(semctl(semid, semnum, SETVAL, args) == -1)
        perror("semctl");
}