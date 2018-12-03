#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/sem.h>
#include <sys/types.h>

int testargs(int);
void unlock(int, int, int);
void lock(int, int, int);
void read_word(int);
void wirte_word(int);
void translate_word();

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
} args;

// Pipe ids
int pip1id, pip2id;

// End of file flag
bool eof = false;

// Word buffers
const int gBufLen = 64;
int gCurWordLen;
char* gReadBuf;
char* gWriteBuf;

int main(int argc, char** argv)
{
    std::cout << "[prog2] Starting program 2" << std::endl;

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

    // Get pipe ids
    pip1id = atoi(argv[0]);
    pip2id = atoi(argv[1]);

    // Initialize buffers
    gReadBuf = (char*)calloc(gBufLen+1, sizeof(char));
    gWriteBuf = (char*)calloc(gBufLen+1, sizeof(char));
    
    while(!eof)
    {
        // Read from input pipe        
        unlock(semid, 0, 1);
        std::cout << "[prog2] unlock" << std::endl;
        read_word(pip1id);
        lock(semid, 0, 0);
        std::cout << "[prog2] lock" << std::endl;

        // translate_word();

        // // Write to output pipe
        // unlock(semid, 1, 0);
        // write_word(pip2id);
        // lock(semid, 1, 1);
    }

    free(gReadBuf);
    free(gWriteBuf);

    std::cout << "[prog2] Exiting..." << std::endl;

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

void read_word(int pipid)
{
    std::cout << "[prog2] read fromp pipe1" << std::endl;

    char temp[1];
    int charCount = 0;
    bool eow = false;
    while(!eow)
    {        
        if(read(pipid, temp, 1) == -1)
        {
            std::cout << "[prog2] error" << std::endl;
            perror("read");
            return;
        }        
        if(temp[0] == '\0')
        {
            std::cout << "[prog2] eow" << std::endl;
            eow = true;
        }
        if(temp[0] == '@')
        {
            std::cout << "[prog2] eof" << std::endl;
            eof = true;
        }
        gReadBuf[charCount] = temp[0];
        charCount++;
    }

    std::cout << "[prog2] " << gReadBuf << std::endl;

    gCurWordLen = charCount-1;
    charCount = 0;
}

void write_word(int pipid)
{

}