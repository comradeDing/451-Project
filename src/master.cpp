#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <errno.h>

/**
 * Custom structs
 */ 
// Program communications
// contains all filenames, pipeids, semaphore and shared memory keys and ids
struct progcomms {
    char* infile;
    char* outfile;
    int pipids[2][2];
    key_t semkey;
    int semid;
    key_t shmkey;
    int shmid;
};

// Executable arguments. Command line arguments to use in execv function
struct execargs {
    char** prog1args;
    char** prog2args;
    char** prog3args;
};
// Semun union used when acessing system v semaphores.
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
} sem_init;

/**
 * Method headers
 */ 
int testargs(int);
int testfile(char*);
execargs set_args(progcomms*);
char* int_to_charptr(int);
void close_prog(int, struct progcomms*);

/**
 * Executable file paths
 */ 
const char* gProg1exe = "./build/PROG1";
const char* gProg2exe = "./build/PROG2";
const char* gProg3exe = "./build/PROG1";

/**
 * FUNCTION: main
 * -----------------------------------
 * configures and initializes all semaphores, pipes and shared memeory, then launches and joins all child processes.
 * closes out all semaphores, pipes and shared memory.
 * 
 * argc: number of command line arguments
 * argv: the command line arguments
 */ 
int main(int argc, char** argv)
{

    std::cout << "[master] pid #" << getpid() << std::endl;
    
    // test args
    if(testargs(argc))
        exit(1);

    // test filename
    if(testfile(argv[1]))
        exit(1);

    // Master data
    pid_t pids[3];      // process IDs
    progcomms md;       // program communications structure
    md.infile = argv[1];
    md.outfile = argv[2];

    // Create 2 pipes
    if(pipe(md.pipids[0]) == -1 || pipe(md.pipids[1]) == -1)
    {
        perror("pipe");
        close_prog(2, &md);
    }

    // create key for semaphore
    if((md.semkey = ftok("./src/master.cpp", 'E')) == -1)
    {
        perror("ftok");
        close_prog(3, &md);
    }

    // initialize semaphore set and get it's id
    if((md.semid = semget(md.semkey, 2, 0666 | IPC_CREAT)) == -1)
    {
        perror("semget");
        close_prog(3, &md);
    }

    // create key for shared memory
    if((md.shmkey = ftok("./src/progs/program1.cpp", 'R')) == -1)
    {
        perror("ftok");
        close_prog(2, &md);
    }

    // initialize shared memeor and get it's id
    if((md.shmid = shmget(md.shmkey, sizeof(int) * 2, 0666 | IPC_CREAT)) == -1)
    {
        perror("shmget");
        close_prog(2, &md);
    }    

    // Set starting values of semaphores
    ushort vals[2] = {0};
    sem_init.array = vals;
    if(semctl(md.semid, 0, SETALL, sem_init) == -1)
    {
        perror("semctl");
        close_prog(4, &md);
    }

    // ***************************************************
    // Fork worker processes *****************************
    // ***************************************************
    
    // populate program argument char arrays
    execargs args = set_args(&md);

    // Fork prog1 ****************************************
    if((pids[0] = fork()) == -1)
    {
        perror("fork");
        exit(5);
    }
    else if(pids[0] == 0) // child process 1
    {   
        // Execute child process 1
        if(execvp(gProg1exe, args.prog1args) == -1)
        {
            perror("execvp failure");
            exit(5);
        }
        exit(0);
    }

    // Fork prog2 ****************************************
    if((pids[1] = fork()) == -1)
    { 
        perror("fork");
        exit(6);
    }
    else if(pids[1] == 0) // child process 2
    {
        // Execute child process 2
        if(execv("./build/PROG2", args.prog2args) == -1)
        {
            perror("execv failure");
            exit(6);
        }
        exit(0);
    }

    // Fork prog3 ****************************************
    if((pids[2] = fork()) == -1)
    {
        perror("fork");
        close_prog(7, &md);
    }
    else if(pids[2] == 0) // child process 3
    {
        // Execute child process 3
        if(execv("./build/PROG3", args.prog3args) == -1)
        {
            perror("execv failure");
            close_prog(7, &md);
        }
        exit(0);
    }

    // wait for worker processes *************************
    int status = WEXITED;
    for(int i = 0; i < 3; i++)
        waitpid(pids[i], &status, 0);    

    // Close and clean up
    close_prog(0, &md);    
}

/**
 * FUNCTION: testargs
 * -------------------------------------
 * Tests if there are the correct number of command line arguments
 * 
 * argc: number of arguments to check against
 */ 
int testargs(int argc)
{
    if(argc != 3)
    {
        std::cout << "Usage: ./MASTER <input filename> <output filename>" << std::endl;
        return 1;
    }
    return 0;
}

/**
 * FUNCTION: testfile
 * -------------------------------------
 * Tests if the input file exists
 * 
 * filename: path of the file to check for
 */ 
int testfile(char* filename)
{
    FILE* fid;
    if(!(fid = fopen(filename, "r")))
    {
        perror("fopen");
        return 1;
    }
    fclose(fid);
    return 0;
}

/**
 * FUNCTION: set_args
 * -------------------------------------
 * Converts master data (md = process communications struct) into command line arguments (char*)
 * 
 * md: The program communications data structure to convert
 */ 
execargs set_args(progcomms *md)
{
    execargs args;
    // program 1 needs filename, pipe 1 write id, semaphore key
    args.prog1args = (char**)calloc(4, sizeof(char*));
    args.prog1args[0] = md->infile;
    args.prog1args[1] = int_to_charptr(md->pipids[0][1]); // write end of pipe 1
    args.prog1args[2] = int_to_charptr((int)md->semkey);
    args.prog1args[3] = NULL;

    args.prog2args = (char**)calloc(5, sizeof(char*));
    args.prog2args[0] = int_to_charptr(md->pipids[0][0]); // read end of pipe 1
    args.prog2args[1] = int_to_charptr(md->pipids[1][1]); // write end of pipe 2
    args.prog2args[2] = int_to_charptr((int)md->semkey);
    args.prog2args[3] = int_to_charptr((int)md->shmkey);
    args.prog2args[4] = NULL;

    args.prog3args = (char**)calloc(5, sizeof(char*));
    args.prog3args[0] = int_to_charptr(md->pipids[1][0]); // read end of pipe 2
    args.prog3args[1] = int_to_charptr((int)md->semkey);
    args.prog3args[2] = int_to_charptr((int)md->shmkey);
    args.prog3args[3] = md->outfile;
    args.prog3args[4] = NULL;

    return args;

}

/**
 * FUNCTION: int_to_charptr
 * -------------------------------------
 * 
 * i: the integer to convert to char pointer (string)
 */ 
char* int_to_charptr(int i)
{
    std::string str = std::to_string(i);
    char* cstr = new char[str.length() + 1];
    strcpy(cstr, str.c_str());
    return cstr;
}

/**
 * FUNCTION: close_prog
 * -------------------------------------
 * closes all of the process communications and exits the program
 * 
 * exitcode: the exit code with which to exit the program
 * md: the data structure containing process communications that need to close
 */ 
void close_prog(int exitcode, struct progcomms *md)
{

    std::cout << "[master] closing pipes" << std::endl;
    // for testing, close pipes
    for(int i = 0; i < 2; i++)
    {
        close(md->pipids[i][0]);
        close(md->pipids[i][1]);
    }

    std::cout << "[master] freeing semaphores" << std::endl;
    // close semaphore set
    semctl(md->semid, 0, IPC_RMID);

    std::cout << "[master] freeing shared memory" << std::endl;
    // close shared memory
    shmctl(md->shmid, IPC_RMID, NULL);

    std::cout << "[master] Ending program..." << std::endl;
    exit(exitcode);
}