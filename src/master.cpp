#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

int testargs(int);
int testfile(char*);

int main(int argc, char** argv)
{

    std::cout << "[master] pid #" << getpid() << std::endl;
    
    // test args
    if(testargs(argc))
        exit(1);

    // test filename
    if(testfile(argv[1]))
        exit(1);
    
    char* infilename = argv[1];

    // Master data
    pid_t pids[3];      // process IDs
    int pipids[2][2];   // pipe IDs
    key_t semkey;       // sys V semaphore key
    int semid;          // sys V semaphore id
    key_t shmkey;       // shared memory key

    // Create 2 pipes
    if(pipe(pipids[0]) == -1 || pipe(pipids[1]) == -1)
    {
        perror("pipe");
        exit(2);
    }

    // create key for semaphore
    if((semkey = ftok(infilename, 'E')) == -1)
    {
        perror("ftok");
        exit(3);
    }

    // initialize semaphore set
    if((semid = semget(semkey, 2, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
    {
        perror("semget");
        exit(3);
    }

    // Fork worker processes
        // Fork process one
    if((pids[0] = fork()) == -1)
    {
        perror("fork");
        exit(4);
    }
    else if(pids[0] == 0) // child process
    {
        execv("./build/PROG1", {"mst"})
    }
        // Fork process two
        // Forks process three

    // wait for worker processes

    // for testing, close pipes
    for(int i = 0; i < 2; i++)
    {
        close(pipids[i][0]);
        close(pipids[i][1]);
    }

    // close semaphore set
    semctl(semid, 0, IPC_RMID);

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