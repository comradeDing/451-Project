#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/shm.h>

int testargs(int);
void unlock(int, int, int);
void lock(int, int, int);
void read_word(int);
void write_word(int);
void translate_word();

char *piglatinize(char *english);
int isVowel(char ch);
int isCap(char *word);
void reCap(char *word);

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

// Count values
int gNumCon;
int gNumVow;

// Shared memory segment
int *counts;

int main(int argc, char** argv)
{
    std::cout << "[prog2] Starting -- pid #" << getpid() << std::endl;

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

    // Get shared memory id
    key_t shmkey = atoi(argv[3]);
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

    // Get pipe ids
    pip1id = atoi(argv[0]);
    pip2id = atoi(argv[1]);

    // Initialize buffers
    gReadBuf = (char*)calloc(gBufLen+1, sizeof(char));
    gWriteBuf = (char*)calloc(gBufLen+1, sizeof(char));
    
    std::cout << "[prog2] start read/write" << std::endl;
    while(!eof)
    {
        // Read from input pipe        
        unlock(semid, 0, 1);
        read_word(pip1id);
        lock(semid, 0, 0);

        translate_word();

        // Write to output pipe
        unlock(semid, 1, 0);
        write_word(pip2id);
        lock(semid, 1, 1);
    }
    std::cout << "[prog2] end read/write" << std::endl;

    counts[0] = gNumVow;
    counts[1] = gNumCon;

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
            eow = true;
        }
        if(temp[0] == '@')
        {
            eof = true;
        }
        gReadBuf[charCount] = temp[0];
        charCount++;
    }

    gCurWordLen = charCount;
    charCount = 0;
}

void write_word(int pipid)
{
    if(!eof)
    {
        write(pipid, gWriteBuf, gCurWordLen);
        return;
    }    
    std::cout << "[prog2] writing eof" << std::endl;
    write(pipid, "@\0", 2); // End file characters    
}

void translate_word()
{    
    if(gReadBuf[0] == '@') return;

    char engl[strlen(gReadBuf)];
    strcpy(engl, gReadBuf);

    char *pigl = piglatinize(engl);

    gCurWordLen = strlen(pigl) + 1;
    strcpy(gWriteBuf, pigl);

    free(pigl);
}

char *piglatinize(char *english)
{
    int engLen = strlen(english);   // Length of english string
    int pigLen = engLen + 3;        // Length of piglatin string
    char chfirst;                   // First letter of piglatin suffix

    // Check if word is capitalized
    int iscap = isCap(english);
    
    if(isVowel(english[0])) 
    {
        gNumVow++;
        // Initalize vowel piglatin string
        pigLen++;
        chfirst = 'r';             
    }
    else
    {
        gNumCon++;
        // "pop" the first letter of the english string
        chfirst = english[0];
        english++;
        engLen--;
    }

    // Initialize piglatin string
    char *piglatin;
    piglatin = (char*)calloc(pigLen, sizeof(char));

    // Initialize loop variables
    int haspunct = 0;
    int lett;
    // Copy letters loop
    for(lett = 0; lett < engLen; lett++)
    {
        // If letter in english string is a punctuation, break the loop
        if(ispunct(english[lett]))
        {
            haspunct = 1;
            break;
        }
        piglatin[lett] = english[lett];
    }
    if(haspunct) // String has punctuation, put it at end and cap with null terminator.
    {            
        piglatin[lett+3] = english[lett];
        piglatin[lett+4] = ' ';
        piglatin[lett+5] = '\0';
    }
    else // No punctuation, cap with null terminator
    {
        piglatin[lett+3] = ' ';
        piglatin[lett+4] = '\0';
    }

    // Add piglatin suffix
    piglatin[lett] = chfirst;
    piglatin[lett+1] = 'a';
    piglatin[lett+2] = 'y';

    // Recapitalize word if needed
    if(iscap)
        reCap(piglatin);

    return piglatin;
}

int isVowel(char ch)
{
    if( ch == 'a' || ch == 'A' ||
        ch == 'e' || ch == 'E' ||
        ch == 'i' || ch == 'I' ||
        ch == 'o' || ch == 'O' ||
        ch == 'u' || ch == 'U')
        return 1;
    else
        return 0;
}

int isCap(char *word)
{
    if('A' <= word[0] && word[0] <= 'Z') return 1;
    return 0;
}

void reCap(char *word)
{
    for(int i = 0; i < strlen(word); i++)
    {
        if(ispunct(word[i])) break;
        word[i] = tolower(word[i]);
    }
    word[0] = toupper(word[0]);
}