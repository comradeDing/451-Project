#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int testargs(int);

int main(int argc, char** argv)
{
    if(testargs(argc))
        exit(1);

    // Get semaphore id

    // process file loop
        // read one word
        // write

    std::cout << "[prog1] pid #" << getpid() << std::endl;
    std::cout << "[prog1] filename '" << argv[0] << "'" << std::endl;
    std::cout << "[prog1] write pipe id #" << argv[1] << std::endl;
    std::cout << "[prog1] sem key #" << argv[2] << std::endl;
    
    for(int i = 1; i <= 5; i++)
    {
        std::cout << "[prog1] " << i << std::endl;
        sleep(1);
    }
        
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