#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/sem.h>
#include <sys/types.h>

int testargs(int);
void unlock(int);
void lock(int);

/**
 * Read buffer
 */ 
char* gReadBuf;
int gReadSize;

void w_read(const int fileHandle);
void w_writeToken(const int fileHandle);
void expandReadBuf(int newSize);

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

    // Initialize read buffer
    //gReadSize = 64;
    //gReadBuf = (char*)calloc(gReadSize+1, sizeof(char)); 

    // process file loop
        // read one word
        // write
    
    for(int i = 1; i <= 5; i++)
    {
        unlock(semid);
        std::cout << "[prog1] " << i << std::endl;
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
    if(argc != 3)
    {
        std::cout << "Usage: ./PROG1 <input filename> <write pipe 1 id> <semaphore key>" << std::endl;
        return 1;
    }
    return 0;
}

void unlock(int semid)
{   
    int semval;
    while((semval = semctl(semid, 0, GETVAL, args)) != 0);
    if(semval == -1)
    {
        perror("semctl");
    }
}

void lock(int semid)
{
    args.val = 1;
    if(semctl(semid, 0, SETVAL, args) == -1)
        perror("semctl");
}