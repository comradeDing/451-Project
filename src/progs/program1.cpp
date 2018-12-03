#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/sem.h>
#include <sys/types.h>

/**
 * Method header declarations
 */
int testargs(int);
void unlock(int);
void lock(int);
void read_word(int);
void write_word(int);
void write_eof(int);

/**
 * Global read buffer and data
 */ 
const int gBufLen = 64;
int gCurWordLen;
char* gReadBuf;

/**
 * EOF flag
 */ 
bool eof = false;

/**
 * Semun data structure for sys V semaphore access
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
    int infileid = open(filename, O_RDONLY);
    
    std::cout << "[prog1] start read/write" << std::endl;
    while(!eof)
    {        
        
        read_word(infileid);

        lock(semid);
        write_word(pipid);
        unlock(semid);
    }

    // Write the eof to the pipe
    lock(semid);
    write_eof(pipid);
    unlock(semid);

    std::cout << "[prog1] end read/write" << std::endl;
        
    free(gReadBuf);
    
    std::cout << "[prog1] Exiting..." << std::endl;

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
    if(argc != 3)
    {
        std::cout << "Usage: ./PROG1 <input filename> <write pipe 1 id> <semaphore key>" << std::endl;
        return 1;
    }
    return 0;
}

/**
 * FUNCTION: lock
 * -------------------------------------
 * wrapper funciton to handle wait and lock of semaphore 0
 *
 * semid: id of the semaphore
 */ 
void lock(int semid)
{   
    int semval;
    while((semval = semctl(semid, 0, GETVAL, args)) != 0);
    if(semval == -1)
    {
        perror("semctl");
    }
}

/**
 * FUNCTION: unlock
 * -------------------------------------
 * wrapper function to handle unlocking of semaphore 0
 * 
 * semid: id of the semaphore
 */ 
void unlock(int semid)
{
    args.val = 1;
    if(semctl(semid, 0, SETVAL, args) == -1)
        perror("semctl");
}

/**
 * FUNCTION: read_word
 * -------------------------------------
 * reads a word ended by a space character from the file associated with a infileid
 * 
 * infileid: the id of the input file
 */ 
void read_word(int infileid)
{

    char temp[1];
    int charCount = 0;
    bool eow = false;
    while(!eow)
    {          
        // Read one character 
        if(read(infileid, temp, 1) == 0)
        {
            std::cout << "[prog1] eof" << std::endl;
            eof = true;
            temp[0] = ' ';
        }
        
        if(temp[0] == ' ')  // If not the end of the word, add to read buff
            eow = true;    
        
        gReadBuf[charCount] = temp[0];            
        charCount++;
    }
    // Cap end of word with null char
    gReadBuf[charCount-1] = '\0';
    gCurWordLen = charCount;    
    charCount = 0;
}

/**
 * FUNCTION: write_word
 * -------------------------------------
 * writes the word contained in gReadBuf to the IO associated with pipid
 * 
 * pipid: the id of the IO
 */ 
void write_word(int pipid)
{
    write(pipid, gReadBuf, gCurWordLen);
}

/**
 * FUNCTION: write_eof
 * -------------------------------------
 * writes eof character (@) to the IO associated with pipid
 * 
 * pipid: the id of the IO
 */ 
void write_eof(int pipid)
{
    std::cout << "[prog1] writing eof" << std::endl;
    write(pipid, "@\0", 2); // End file characters
}