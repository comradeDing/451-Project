#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <sys/types.h>

#define DATAPATH "./data/"

int testargs(int);
void unlock(int);
void lock(int);
void read_word(int);
void write_word(int);

// End of file flag
bool eof = false;

// Word buffers
const int gBufLen = 64;
int gCurWordLen;
char* gReadBuf;

// pipe, sharedmem and outfile ids
int pip1id;
int shmid;
int outfileid;

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
} args;

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

    // Get shmid
    shmid = atoi(argv[2]);

    // Get semaphore id
    key_t semkey = atoi(argv[1]);
    int semid;
    if((semid = semget(semkey, 0, 0)) == -1)
    {
        perror("semget");
        exit(1);
    }    
    
    // Initialize read buffer
    gReadBuf = (char*)calloc(gBufLen+1, sizeof(char*));

    std::cout << "[prog3] start read/write" << std::endl;
    while(!eof)
    {

        unlock(semid);
        read_word(pip1id);
        lock(semid);
        
        write_word(outfileid);        

    }
    std::cout << "[prog3] end read/write" << std::endl;

    // Cleanup
    close(outfileid);    
    free(gReadBuf);

    std::cout << "[prog3] Exiting..." << std::endl;

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
        std::cout << "Usage: ./PROG3 <read pipe 2 id> <semaphore key> <shared mem key> <output filename>" << std::endl;
        return 1;
    }
    return 0;
}

void unlock(int semid)
{   
    int semval;
    while((semval = semctl(semid, 1, GETVAL, args)) != 1);
    if(semval == -1)
    {
        perror("semctl");
    }
}

void lock(int semid)
{
    args.val = 0;
    if(semctl(semid, 1, SETVAL, args) == -1)
        perror("semctl");
}

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