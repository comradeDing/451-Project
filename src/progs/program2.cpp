#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/shm.h>

/**
 * Method headers
 */ 
int testargs(int);
void unlock(int, int, int);
void lock(int, int, int);
void read_word(int);
void write_word(int);
void translate_word();

// translation methods
char *piglatinize(char *english);
int isVowel(char ch);
int isCap(char *word);
void reCap(char *word);

/**
 * Structs
 */  
// Union needed access system v semapores
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
} args;

/**
 * Pipe IDs
 */ 
int pip1id, pip2id;

/**
 * End of file flag
 */ 
bool eof = false;

/**
 * Input and output word buffers
 */ 
const int gBufLen = 64;
int gCurWordLen;
char* gReadBuf;
char* gWriteBuf;

/**
 * Shared memory
 */ 
// Data to share
int gNumCon;
int gNumVow;
// Shared memory segment
int *counts;

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
        lock(semid, 0, 1);
        read_word(pip1id);
        unlock(semid, 0, 0);

        translate_word();
        
        // Set shared memory
        counts[0] = gNumVow;
        counts[1] = gNumCon;
        
        // Write to output pipe
        lock(semid, 1, 0);
        write_word(pip2id);
        unlock(semid, 1, 1);
    }
    std::cout << "[prog2] end read/write" << std::endl;

    // Clean up
    free(gReadBuf);
    free(gWriteBuf);

    std::cout << "[prog2] Exiting..." << std::endl;

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
        std::cout << "Usage: ./PROG2 <read pipe 1 id> <write pipe 2 id> <semaphore key> <shared mem key>" << std::endl;
        return 1;
    }
    return 0;
}

/**
 * FUNCTION: lock
 * -------------------------------------
 * wrapper funciton to handle wait and lock of semaphores
 *
 * semid: id of the semaphore
 * semnum: the semaphore being locked
 * semval: the blocking value of the semaphore
 */ 
void lock(int semid, int semnum, int semval)
{
    int val;
    while((val = semctl(semid, semnum, GETVAL, args)) != semval);
    if(val == -1)
    {
        perror("semctl");
    }
}

/**
 * FUNCTION: unlock
 * -------------------------------------
 * wrapper function to handle unlocking of semaphores
 * 
 * semid: id of the semaphore
 * semnum: the semaphore being unlocked
 * semval: the value to set the semaphore to
 */ 
void unlock(int semid, int semnum, int semval)
{
    args.val = semval;
    if(semctl(semid, semnum, SETVAL, args) == -1)
        perror("semctl");
}

/**
 * FUNCTION: read_word
 * -------------------------------------
 * reads a word ended by a space character from the IO associated with a pipid
 * 
 * pipid: the id of the IO
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

/**
 * FUNCTION: write_word
 * -------------------------------------
 * writes the word contained in gReadBuf to the IO associated with pipid
 * 
 * pipid: the id of the IO
 */ 
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

/**
 * FUNCTION: translate_word
 * -------------------------------------
 * translates the word located in gReadBuf from english to piglatin and copies it to gWriteBuf
 */ 
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

/**
 * FUNCTION: piglatinize
 * -------------------------------------
 * Translates a input string from english to piglatin and updates the number of voweled and consanant words (global variables)
 * 
 * engilsh: the input english word to translate
 * 
 * returns the translated piglatin word
 */ 
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

/**
 * FUNCTION: isVowel
 * -------------------------------------
 * checks if the input char is a vowel. ignores case of character
 * 
 * ch: the character to check
 *  
 * returns 1 if a vowel, 0 if not
 */ 
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

/**
 * FUNCTION: isCap
 * -------------------------------------
 * Checks if the word is capitalized
 * 
 * word: the word to check
 * 
 * returns 1 if capitalized and 0 if not
 */ 
int isCap(char *word)
{
    if('A' <= word[0] && word[0] <= 'Z') return 1;
    return 0;
}
/**
 * FUNCTION: isVowel
 * -------------------------------------
 * recapitalizes a translated piglatin word
 * 
 * word: the word to recapitalize
 */ 
void reCap(char *word)
{
    for(int i = 0; i < strlen(word); i++)
    {
        if(ispunct(word[i])) break;
        word[i] = tolower(word[i]);
    }
    word[0] = toupper(word[0]);
}