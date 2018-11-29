#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

struct progcomms {
    char* infile;
    char* outfile;
    int pipids[2][2];
    key_t semkey;
    int semid;
    key_t shmkey;
};

struct execargs {
    char** prog1args;
    char** prog2args;
    char** prog3args;
};

int testargs(int);
int testfile(char*);
execargs set_args(progcomms*);
char* int_to_charptr(int);

void close_prog(int);

const char* gProg1exe = "./build/PROG1";
const char* gProg2exe = "./build/PROG2";
const char* gProg3exe = "./build/PROG1";

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
    // int pipids[2][2];   // pipe IDs
    // key_t semkey;       // sys V semaphore key
    // int semid;          // sys V semaphore id
    // key_t shmkey;       // shared memory key
    progcomms md;
    md.infile = argv[1];
    md.outfile = argv[2];

    // Create 2 pipes
    if(pipe(md.pipids[0]) == -1 || pipe(md.pipids[1]) == -1)
    {
        perror("pipe");
        exit(2);
    }

    // create key for semaphore
    if((md.semkey = ftok(md.infile, 'E')) == -1)
    {
        perror("ftok");
        exit(3);
    }

    // initialize semaphore set
    if((md.semid = semget(md.semkey, 2, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
    {
        perror("semget");
        exit(3);
    }

    execargs args = set_args(&md);

    // Fork worker processes
    
    // Fork prog1
    if((pids[0] = fork()) == -1)
    {
        perror("fork");
        exit(4);
    }
    else if(pids[0] == 0) // child process 1
    {
        if(execvp(gProg1exe, args.prog1args) == -1)
        {
            perror("execvp failure");
            exit(4);
        }
        std::cout << "[prog1] Ending process" << std::endl;
        exit(0);
    }

    // Fork prog2
    if((pids[1] = fork()) == -1)
    {
        perror("fork");
        exit(4);
    }
    else if(pids[1] == 0) // child process 2
    {
        if(execv("./build/PROG2", args.prog2args) == -1)
        {
            perror("execv failure");
            exit(4);
        }
        std::cout << "[prog2] Ending process" << std::endl;
        exit(0);
    }

    // Fork prog3
    if((pids[2] = fork()) == -1)
    {
        perror("fork");
        exit(4);
    }
    else if(pids[2] == 0) // child process 3
    {
        if(execv("./build/PROG3", args.prog3args) == -1)
        {
            perror("execv failure");
            exit(4);
        }
        std::cout << "[prog3] Ending process" << std::endl;
        exit(0);
    }

    // wait for worker processes
    int status = WEXITED;
    for(int i = 0; i < 3; i++)
        waitpid(pids[i], &status, 0);

    // for testing, close pipes
    for(int i = 0; i < 2; i++)
    {
        close(md.pipids[i][0]);
        close(md.pipids[i][1]);
    }

    // close semaphore set
    semctl(md.semid, 0, IPC_RMID);

    std::cout << "[master] ending program..." << std::endl;
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
        std::cout << "Usage: ./MASTER <input filename> <output filename>" << std::endl;
        return 1;
    }
    return 0;
}

/**
 * FUNCTION: testfile
 * -------------------------
 * Tests if the input file exists
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

execargs set_args(progcomms *md)
{
    execargs args;
    // program 1 needs filename, pipe 1 write id, semaphore key
    args.prog1args = (char**)calloc(4, sizeof(char*));
    args.prog1args[0] = md->infile;
    args.prog1args[1] = int_to_charptr(md->pipids[0][0]);
    args.prog1args[2] = int_to_charptr((int)md->semkey);
    args.prog1args[3] = NULL;

    args.prog2args = (char**)calloc(5, sizeof(char*));
    args.prog2args[0] = int_to_charptr(md->pipids[0][1]);
    args.prog2args[1] = int_to_charptr(md->pipids[1][0]);
    args.prog2args[2] = int_to_charptr((int)md->semkey);
    args.prog2args[3] = int_to_charptr((int)md->shmkey);
    args.prog2args[4] = NULL;

    args.prog3args = (char**)calloc(5, sizeof(char*));
    args.prog3args[0] = int_to_charptr(md->pipids[1][1]);
    args.prog3args[1] = int_to_charptr((int)md->semkey);
    args.prog3args[2] = int_to_charptr((int)md->shmkey);
    args.prog3args[3] = md->outfile;
    args.prog3args[4] = NULL;

    return args;

}

char* int_to_charptr(int i)
{
    std::string str = std::to_string(i);
    char* cstr = new char[str.length() + 1];
    strcpy(cstr, str.c_str());
    return cstr;
}