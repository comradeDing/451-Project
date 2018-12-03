#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/sem.h>
#include <sys/types.h>


int testargs(int);
void unlock(int);
void lock(int);

/**
 * Global read variables
 */ 
const int gBufLen = 64;
int gCurWordLen;
char* gReadBuf;
bool eof = false;

void read_word(int);
void write_word(int);
void write_eof(int);

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
} args;

int main(int argc, char** argv)
{
    std::cout << "[prog1] Starting -- pid #" << getpid() << std::endl;

    if(testargs(argc))
        exit(1);
    
    // Build filename
    char* filename = argv[0];
    std::cout << "[prog1] Input file path: " << filename << std::endl;

    // Get write pipe 1 id
    int pipid = atoi(argv[1]);

    // Get semaphore id
    key_t semkey = atoi(argv[2]);
    int semid;
    if((semid = semget(semkey, 0, 0)) == -1)
    {
        perror("semget");
        exit(1);
    }

    // Initialize read buffer
    gReadBuf = (char*)calloc(gBufLen+1, sizeof(char));

    // Initialize input file and write pipe
    int filehandle = open(filename, O_RDONLY);
    
    std::cout << "[prog1] start read/write" << std::endl;
    while(!eof)
    {        
        
        read_word(filehandle);

        unlock(semid);
        write_word(pipid);
        lock(semid);
    }

    // Write the eof to the pipe
    unlock(semid);
    write_eof(pipid);
    lock(semid);

    std::cout << "[prog1] end read/write" << std::endl;
        
    free(gReadBuf);
    
    std::cout << "[prog1] Exiting..." << std::endl;

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

void read_word(int filehandle)
{

    char temp[1];
    int charCount = 0;
    bool eow = false;
    while(!eow)
    {          
        // Read one character 
        if(read(filehandle, temp, 1) == 0)
        {
            std::cout << "[prog1] eof" << std::endl;
            eof = true;
            temp[0] = ' ';
        }
        if(temp[0] != ' ')  // If not the end of the word, add to read buff
        {
                        
            gReadBuf[charCount] = temp[0];            
        }
        else                // If the end of the word, trip end of word flag
            eow = true;

        charCount++;
    }
    // Cap end of word with null char
    gReadBuf[charCount-1] = '\0';
    gCurWordLen = charCount;    
    charCount = 0;
}

void write_word(int pipid)
{
    write(pipid, gReadBuf, gCurWordLen);
}

void write_eof(int pipid)
{
    std::cout << "[prog1] writing eof" << std::endl;
    write(pipid, "@\0", 2); // End file characters
}