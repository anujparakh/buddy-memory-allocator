#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include<string.h>
#include <cstring>
#include "Ackerman.h"
#include "BuddyAllocator.h"

void easytest(BuddyAllocator *ba)
{
    // Test 4 differently sized allocations
    // And free in 2 steps
    // Also print list after every step
    
    cout << "******** BEGIN EASY TEST ********" << endl << endl;
    cout << "**** No allocations ****" << endl << endl;
    ba->printlist();
    cout << endl << endl;
    
    cout << "**** Allocating 52 bytes (mem1) ****" << endl << endl;
    char *mem1 = ba->alloc(52);
    if (!mem1)
    {
        return;
    }
    
    memset(mem1, 'c', 52);
    ba->printlist();
    cout << endl << endl;

    cout << "**** Allocating 234 bytes (mem2) ****" << endl << endl;

    char *mem2 = ba->alloc(234);
    if (!mem2)
    {
        return;
    }
    memset(mem2, 'c', 234);

    ba->printlist();
    cout << endl << endl;

    cout << "**** Allocating 400 bytes (mem3) ****" << endl << endl;
    char *mem3 = ba->alloc(400);
    if (!mem3)
    {
        return;
    }
    memset(mem3, 'c', 400);

    ba->printlist();
    cout << endl << endl;
    
    cout << "**** Allocating 400 bytes (mem4) ****" << endl << endl;
    char *mem4 = ba->alloc(400);
    if (!mem4)
    {
        return;
    }
    memset(mem4, 'c', 400);

    // now print again, how should the list look now
    ba->printlist();
    
    cout << "**** Freeing mem1 and mem2 ****" << endl << endl;
    ba->free(mem1);
    ba->printlist();
    ba->free(mem2);
    ba->printlist();
    cout << endl;
    
    cout << "**** FREEING MEM3 and MEM4****" << endl << endl;
    ba->free(mem3);
    
    ba->printlist();
    ba->free(mem4);

    ba->printlist(); // shouldn't the list now look like as in the beginning
}

// Helper function to check if given number is a power of 2
bool isPowerOfTwo (int toCheck)
{
    // Negative numbers and 0 not allowed :)
    if (toCheck <= 0)
        return false;
    return (ceil(log2(toCheck)) == floor(log2(toCheck)));
}

int main(int argc, char **argv)
{
    // Set default values (128 bytes, and 512 KB)
    int basicBlockSize = 128, memoryLength = 512 * 1024;

    // Parse arguments
    // To use for argument
    int opt;
    
    try
    {
        while((opt = getopt(argc, argv, "b:s:")) != -1)
        {
            switch(opt)
            {
                case 'b':
                    basicBlockSize = atoi(optarg);
                    break;
                case 's':
                    memoryLength = atoi(optarg);
                    break;
                default:
                    cout << "Improper Argument Usage." << endl;
                    return 0;
            }
        }
    }
    catch (const std::exception &e)
    {
        cout << "Problem reading arguments. Error: " << e.what() << endl;
    }
    
    cout << "basic block size = " << basicBlockSize << " bytes." << endl;
    cout << "total memory = " << memoryLength  << " bytes." << endl;

    // Make sure arguments are valid
    if (!isPowerOfTwo(basicBlockSize) or (basicBlockSize > memoryLength))
    {
        cout << "Basic Block Size is invalid. It must be a positive power of 2 and smaller than total memory size." << endl;
        return -1;
    }
        
    if (!isPowerOfTwo(memoryLength))
    {
        cout << "Total Memory Size is invalid. It must be a positive power of 2 and a multiple of basic block size." << endl;
        return -1;
    }
    
    // create memory manager
    BuddyAllocator *allocator = new BuddyAllocator(basicBlockSize, memoryLength);
    
    // Simple user-written test for the allocator
    easytest(allocator);
    
    // stress-test the memory manager
    Ackerman *am = new Ackerman();
    am->test(allocator); // this is the full-fledged test.
    
    // destroy memory manager
    delete allocator;
}
