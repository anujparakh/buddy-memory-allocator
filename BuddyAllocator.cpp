#include "BuddyAllocator.h"
#include <math.h>
#include <iostream>
using namespace std;

//
// PUBLIC FUNCTIONS
//

BuddyAllocator::BuddyAllocator(int _basic_block_size, int _total_memory_length): basic_block_size(_basic_block_size), total_memory_size(_total_memory_length)
{
    // Allocate all of the memory
    memoryStart = new char [total_memory_size];
    // Create one huge block with all the memory
    BlockHeader *blockHeaderStart = (BlockHeader*) memoryStart;
    blockHeaderStart->blockSize = total_memory_size;
    blockHeaderStart->next = nullptr;
    blockHeaderStart->free = true;
    
    // Add Lists of each size in the FreeList
    for (int i = basic_block_size; i <= total_memory_size; i *= 2)
    {
        FreeList.push_back(LinkedList());
    }
    
    // Add the huge block to FreeList
    FreeList [FreeList.size() - 1].insert(blockHeaderStart);
}

BuddyAllocator::~BuddyAllocator()
{
    // set all the BlockHeaders to nullptr so that there are no dangling pointers
    for (auto memoryList: FreeList)
    {
        while (memoryList.head)
        {
            BlockHeader *toDelete = memoryList.head;
            memoryList.head = memoryList.head->next;
            toDelete = nullptr;
        }
    }
    
    // Delete all the allocated memory
    delete [] memoryStart; // this should be the last step
}

char *BuddyAllocator::alloc(int _length)
{
    /* This preliminary implementation simply hands the call over the
     the C standard library!
     Of course this needs to be replaced by your implementation.
     */
    
    // FROM LECTURE
    // int rx = x + sizeof(BH)
    // int i = some func(rx, basic block size)
    // if (FL [i].head != NULL) -> then a block exists
    //   BH * b = FL [i].remove_head();
    //   return (char *)(b + 1);
    // else
    //   // Keep going down until you find FL[j] head != NULL
    //   j++;
    //   if (j >= FL.size()) return 0/NULL; -> issa full
    //   while (j > i)
    //      b = FL[j].head
    //      BH *buddyblock = split (b)
    //      // when you split a block, just put another header in the middle
    //      FL[j].remove(b);
    //      FL[j-1].insert(bb);
    //      // when j = i, then buddy block is small enough
    
    try
    {
        int levelInFreeList = calculateFreeListIndex(_length + sizeof(BlockHeader));
        
        if (FreeList[levelInFreeList].head) // free block of needed size exists
        {
            BlockHeader *toReturn = FreeList[levelInFreeList].head;
            toReturn->free = false;
            FreeList[levelInFreeList].remove(toReturn);
            return (char*) (toReturn + 1);
        }
        
        // Look for an empty block bigger than the block we need
        // start looking at 1 level above the level we need
        int indexFreeList = levelInFreeList + 1;
        while (!FreeList[indexFreeList].head)
        {
            ++indexFreeList;
            if (indexFreeList == FreeList.size())
                return nullptr; // No memory unavailable
        }
        
        // Now we have found a free block
        // Split it down until size we need is attained.
        // And keep waste blocks in FreeList
        while (indexFreeList > levelInFreeList)
        {
            // Split the block
            BlockHeader *toSplit = FreeList[indexFreeList].head;
            BlockHeader *splittedBuddy = split(toSplit);
            if (splittedBuddy == nullptr)
                return nullptr; // some error
            
            // remove the splitted block from the FreeList
            FreeList[indexFreeList].remove(toSplit);
            
            // Add both new blocks to the next FreeList
            FreeList[indexFreeList - 1].insert(toSplit);
            FreeList[indexFreeList - 1].insert(splittedBuddy);
            
            --indexFreeList; // go to next list
        }
        
        // Now indexFreeList is = levelInFreeList
        BlockHeader *toReturn = FreeList[levelInFreeList].head;
        toReturn->free = false;
        FreeList[levelInFreeList].remove(toReturn);
        return (char*) (toReturn + 1);
    }
    catch (const std::exception& e)
    {
        cout << "Exception Occured in Alloc: " << endl;
        return nullptr;
    }
    //    return new char[_length];
}

int BuddyAllocator::free(char *_a)
{
    // BH *b = addr - sizeof(BH)
    // int i = some func(b->bs, bbs)
    // BH *bb = buddyaddress (b) -> calculates buddy of a given block
    // // if buddy is free, merge them together and put them in the linked list
    // while (true)
    // if (isfree (bb)) -> use free bit in block header
    //    BH *m merge (b, bb) -> opposite of split and returns smaller address i.e. whatever is first
    //    FL[i+1].insert(m)
    // else
    //    FL[i].insert(b)
    //    break;
    
    // BH *buddyaddr (BA *b):
    //    NOTE: Use 64-byte integer (long should be good or uint64_t)
    //    return ( (address - start) ^ bs) + start // XOR with the size gives you the next block
    
    try
    {
        // Try to create a blockHeader from the given block
        // Subtracting the size because blockHeader is located before the given memory
        BlockHeader *toDelete = (BlockHeader* ) (_a - sizeof(BlockHeader));
        toDelete->free = true;
        int levelInFreeList = calculateFreeListIndex(toDelete->blockSize);
        BlockHeader *toDeleteBuddy = getbuddy(toDelete);
        // insert the block back into the freeList
        FreeList[levelInFreeList].insert(toDelete);

        while (true)
        {
            if (!toDeleteBuddy->free or (toDelete->blockSize != toDeleteBuddy->blockSize))
            {
                return 0;
            }

            // Merge the two blocks
            // They will get removed from FreeList by merge
            BlockHeader *mergedResult = merge(toDelete, toDeleteBuddy);
            
            FreeList [levelInFreeList + 1].insert(mergedResult);
            
            if (mergedResult->blockSize == total_memory_size) // reached highest level
                return 0;
            
            // Get ready for next iteration
            toDelete = mergedResult;
            toDeleteBuddy = getbuddy(toDelete);
            ++levelInFreeList;
        }
    }
    catch (...)
    {
        cout << "Exception Occured in Free: " <<  endl;
        return -1;
    }
    
    return 0;
}




void BuddyAllocator::printlist()
{
    cout << "Printing the Freelist in the format \"[index] (block size) : # of blocks\"" << endl;
    for (int i = 0; i < FreeList.size(); i++)
    {
        cout << "[" << i << "] (" << ((1 << i) * basic_block_size) << ") : "; // block size at index should always be 2^i * bbs
        int count = 0;
        BlockHeader *b = FreeList[i].head;
        // go through the list from head to tail and count
        while (b)
        {
            count++;
            // block size at index should always be 2^i * bbs
            // checking to make sure that the block is not out of place
            if (b->blockSize != (1 << i) * basic_block_size)
            {
                cerr << "ERROR:: Block is in a wrong list" << endl;
                exit(-1);
            }
            b = b->next;
        }
        cout << count << endl;
    }
}

//
// PRIVATE FUNCTIONS
//

int BuddyAllocator::calculateFreeListIndex (int memorySize)
{
    // If memory is bigger than total memory, CAN'T DO IT
    if (memorySize > total_memory_size)
        return -1;
    
    if (memorySize < basic_block_size)
        return 0;
    
    return ceil (log(memorySize)/log(2) - log(basic_block_size) / log(2));
    
}

// Helper function to return the address of a given block's buddy
BlockHeader * BuddyAllocator::getbuddy(BlockHeader *block)
{
    // No buddy if block is biggest size possible
    if (block->blockSize == total_memory_size)
        return nullptr;
    
    // save address of block and start of allocated memory in uintptr_ts
    uintptr_t addr = (uintptr_t) block;
    uintptr_t start = (uintptr_t) memoryStart;
    // calculate address of budy using XOR and recast it as BlockHeader
    BlockHeader* theBuddy = (BlockHeader *) (((addr - start) ^ block->blockSize) + start);
    return theBuddy;
}

// Helper function to check if two given blocks are buddies
bool BuddyAllocator::arebuddies(BlockHeader *block1, BlockHeader *block2)
{
    // Two blocks are buddies if size is equal, and getbuddy should get you the other block
    return ((block1->blockSize == block2->blockSize) and getbuddy(block1) == block2);
}

// Helper function to merge 2 given blocks into a bigger block
BlockHeader * BuddyAllocator::merge(BlockHeader *block1, BlockHeader *block2)
{
    // Make sure blocks are free and of equal size
//    if (!(block1->free and block2->free and block1->blockSize == block2->blockSize))
//        return nullptr;
    
    // If block2 is before block1, swap them
    if (uintptr_t(block1) > uintptr_t(block2))
    {
        BlockHeader *temp = block1;
        block1 = block2;
        block2 = temp;
    }
    
    // remove the two blocks from free list
    FreeList [calculateFreeListIndex(block1->blockSize)].remove(block1);
    FreeList [calculateFreeListIndex(block2->blockSize)].remove(block2);
    
    // Create the merged block header
    block1->blockSize = block1->blockSize << 1;
    
    // Empty the given block headers
//    memset ((mergedBlock + 1), NULL, mergedBlock->blockSize - sizeof(BlockHeader));
    
    return block1;
}

// Helper function to split a given block if it is empty
BlockHeader * BuddyAllocator::split(BlockHeader *block)
{
    // Make sure the given block is free
    // If not, can't split it
    if (!block->free)
        return nullptr;
    
    // Make the BlockHeader of half the size
    block->blockSize = block->blockSize >> 1;
    
    // find buddy address of the given block and assign it
    // *I THINK* this will add the new block at the right address
    BlockHeader *newBlock = (BlockHeader *) (uintptr_t(block) + block->blockSize);
    
    // assign the block
    newBlock->blockSize = block->blockSize;
    newBlock->free = true;
    
    return newBlock;
}
