#include <unistd.h>
#include <stdlib.h>
#include "Ackerman.h"
#include "BuddyAllocator.h"

void easytest(BuddyAllocator *ba)
{
    // be creative here
    // know what to expect after every allocation/deallocation cycle
    
    cout << "******** BEGIN EASY TEST ********" << endl << endl;
    cout << "**** No allocations ****" << endl << endl;
    ba->printlist();
    cout << endl << endl;
    
    char *mem1 = ba->alloc(52);
    cout << "**** Allocating 52 bytes (mem1) ****" << endl << endl;
    memset(mem1, 'c', 52);
    ba->printlist();
    cout << endl << endl;

    cout << "**** Allocating 234 bytes (mem2) ****" << endl << endl;

    char *mem2 = ba->alloc(234);
    memset(mem2, 'c', 234);

    ba->printlist();
    cout << endl << endl;

    cout << "**** Allocating 400 bytes (mem3) ****" << endl << endl;
    char *mem3 = ba->alloc(400);
    memset(mem3, 'c', 400);

    ba->printlist();
    cout << endl << endl;
    
    cout << "**** Allocating 400 bytes (mem4) ****" << endl << endl;
    char *mem4 = ba->alloc(400);
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

int main(int argc, char **argv)
{
    // Parse the command line arguments using getopt
    // Set default values (128 bytes, and 512 KB)
    int basicBlockSize = 128, memoryLength = 128 * 1024 * 1024;

    // Parse arguments
    try
    {
        for (int argNum = 1; argNum < argc; ++argNum)
        {
            if (strcmp(argv[argNum], "-b") == 0)
                basicBlockSize = atoi(argv[++argNum]);
            else if(strcmp(argv[argNum], "-s") == 0)
                memoryLength = atoi(argv[++argNum]);
        }
    }
    catch (const std::exception& e)
    {
        cout << "Improper argument usage" << endl;
    }
    
    // create memory manager
    BuddyAllocator *allocator = new BuddyAllocator(basicBlockSize, memoryLength);
    
    // the following won't print anything until you start using FreeList and replace the "new" with your own implementation
    //easytest(allocator);
    
    // stress-test the memory manager, do this only after you are done with small test cases
    Ackerman *am = new Ackerman();
    am->test(allocator); // this is the full-fledged test.
    
    // destroy memory manager
    delete allocator;
}
