#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int testargs(int);

int main(int argc, char** argv)
{
    std::cout<< "[prog1] argc = " << argc << std::endl;
    if(testargs(argc))
        exit(1);

    FILE* outfile = fopen("./data/p1output.txt", "w");

    std::cout << "[prog1] pid #" << getpid() << std::endl;
    std::cout << "[prog1] filename '" << argv[0] << "'" << std::endl;
    std::cout << "[prog1] write pipe id #" << argv[1] << std::endl;
    std::cout << "[prog1] sem key #" << argv[2] << std::endl;

    fclose(outfile);

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