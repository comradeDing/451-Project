#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char** argv)
{

    if(testargs(argc))
        exit(1);

    std::cout << "[prog3] pid #" << getpid() << std::endl;
    std::cout << "[prog3] read pipe 2 id #" << argv[1] << std::endl;
    std::cout << "[prog3] sem key #" << argv[2] << std::endl;
    std::cout << "[prog3] shared mem key #" << argv[4] << std::endl;

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
        std::cout << "Usage: ./PROG3 <read pipe 2 id> <semaphore key> <shared mem key>" << std::endl;
        return 1;
    }
    return 0;
}