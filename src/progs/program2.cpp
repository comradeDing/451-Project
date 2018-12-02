#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/sem.h>
#include <sys/types.h>

int testargs(int);
void unlock(int);
void lock(int);

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
        unlock(semid);
        std::cout << "[prog2] " << i << std::endl;
        sleep(1);
        lock(semid);
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

void unlock(int semid)
{
    int semval;
    while((semval = semctl(semid, 0, GETVAL, args)) != 1);
    if(semval == -1)
    {
        perror("semctl");
    }
}

void lock(int semid)
{
    args.val = 0;
    if(semctl(semid, 0, SETVAL, args) == -1)
        perror("semctl");
}