// Erich Dingeldein
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/shm.h>

#define DATAPATH "./data/"

/**
 * Method headers
 */ 
int testargs(int);
void unlock(int);
void lock(int);
void read_word(int);
void write_word(int);

/**
 * End of file flag
 */ 
bool eof = false;

/**
 * IO buffer
 */ 
const int gBufLen = 64;
int gCurWordLen;
char* gReadBuf;

/**
 * Shared memory segment
 */ 
int *counts;

/**
 * Pipe and file id's
 */ 
int pip1id;
int outfileid;

/**
 * Data structure for sys v semaphore access
 */ 
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
} args;

/**
 * FUNCTION: main
 * -------------------------------------
 * Initializes program and runs main read/write loop.
 * 
 * argc: number of command line arguments
 * argv: array of command line arguments
 */
int main(int argc, char** argv)
{

    std::cout << "[prog3] Starting -- pid #" << getpid() << std::endl;

    if(testargs(argc))
        exit(1);

    // Build outputfile path/name
    char outputfilename[128];
    strcpy(outputfilename, DATAPATH);
    strcat(outputfilename, argv[3]);

    // Create output file, save handle
    std::cout << "[prog3] Output file path: " << outputfilename << std::endl;
    if((outfileid = open(outputfilename, O_CREAT | O_WRONLY, 0664)) == -1)
    {
        perror("open");        
    }

    // Get pipid
    pip1id = atoi(argv[0]);

    // Get semaphore id
    key_t semkey = atoi(argv[1]);
    int semid;
    if((semid = semget(semkey, 0, 0)) == -1)
    {
        perror("semget");
        exit(1);
    }    
    
    // Get shared memory id
    key_t shmkey = atoi(argv[2]);
    int shmid;
    if((shmid = shmget(shmkey, 0, 0)) == -1)
    {
        perror("shmget");
        exit(2);
    }

    // Attach to shared memory
    counts = (int *)shmat(shmid, (void*)0, 0);
    if(counts == (int *)(-1))
    {
        perror("shmat");
        exit(2);
    }

    // Initialize read buffer
    gReadBuf = (char*)calloc(gBufLen+1, sizeof(char*));

    std::cout << "[prog3] start read/write" << std::endl;
    while(!eof)
    {

        lock(semid);
        read_word(pip1id);
        unlock(semid);
        
        write_word(outfileid);        

    }
    std::cout << "[prog3] end read/write" << std::endl;

    // Print number of each type of word
    std::cout << "Type 1: " << counts[0] << std::endl;
    std::cout << "Type 2: " << counts[1] << std::endl;

    // Cleanup
    close(outfileid);    
    free(gReadBuf);

    std::cout << "[prog3] Exiting..." << std::endl;

    exit(0);
}

/**
 * FUNCTION: testargs
 * -------------------------------------
 * tests if there are the correct number of command line arguments
 * 
 * argc: number of arguments to test against
 */ 
int testargs(int argc)
{
    if(argc != 4)
    {
        std::cout << "Usage: ./PROG3 <read pipe 2 id> <semaphore key> <shared mem key> <output filename>" << std::endl;
        return 1;
    }
    return 0;
}

/**
 * FUNCTION: lock
 * -------------------------------------
 * wrapper funciton to handle wait and lock of semaphore 1
 *
 * semid: id of the semaphore
 */ 
void lock(int semid)
{   
    int semval;
    while((semval = semctl(semid, 1, GETVAL, args)) != 1);
    if(semval == -1)
    {
        perror("semctl");
    }
}

/**
 * FUNCTION: unlock
 * -------------------------------------
 * wrapper function to handle unlocking of semaphore 1
 * 
 * semid: id of the semaphore
 */ 
void unlock(int semid)
{
    args.val = 0;
    if(semctl(semid, 1, SETVAL, args) == -1)
        perror("semctl");
}

/**
 * FUNCTION: read_word
 * -------------------------------------
 * reads a word ended by a null character from the IO associated with a pipid
 * 
 * infileid: the id of the input IO
 */ 
void read_word(int pipid)
{

    char temp[1];
    int charCount = 0;
    bool eow = false;
    while(!eow)
    {        
        if(read(pipid, temp, 1) == -1)
        {
            perror("read");
            return;
        }        
        if(temp[0] == '\0')
        {
            temp[0] == '0';
            eow = true;
        }
        if(temp[0] == '@')
        {
            eof = true;
        }
        gReadBuf[charCount] = temp[0];
        charCount++;
    }

    // Cap string with null char
    gReadBuf[charCount] = '\0';
    gCurWordLen = charCount;
    charCount = 0;
}

/**
 * FUNCTION: write_word
 * -------------------------------------
 * writes the word contained in gReadBuf to the IO associated with outfileid
 * 
 * outfileid: the id of the IO
 */ 
void write_word(int outfileid)
{
    if(!eof)
    {
        write(outfileid, gReadBuf, gCurWordLen - 1);
        return;
    }    
    std::cout << "[prog3] writing eof" << std::endl;
    write(outfileid, "\n", 1); // End file character   
}