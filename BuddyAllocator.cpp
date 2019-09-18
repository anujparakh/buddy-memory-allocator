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
    // Try catch block to catch stupid exceptions
    // Note: This does not catch BAD_ACCESS and Segmentation Faults
    try
    {
        // Find the level in FreeList needed
        int levelInFreeList = calculateFreeListIndex(_length + sizeof(BlockHeader));
        
        // Check if free block of needed size exist already
        if (FreeList[levelInFreeList].head)
        {
            BlockHeader *toReturn = FreeList[levelInFreeList].head;
            toReturn->free = false;
            FreeList[levelInFreeList].remove(toReturn);
            return (char*) (toReturn + 1);
        }
        
        if (levelInFreeList == FreeList.size() - 1)
            return nullptr; // can't split the biggest block if that's what we need
        
        
        // Look for an empty block bigger than the block we need
        // start looking at 1 level above the level we need
        int indexFreeList = levelInFreeList + 1;
        while (!FreeList[indexFreeList].head)
        {
            ++indexFreeList;
            if (indexFreeList >= FreeList.size())
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
        cout << "Exception Occured in Alloc: " << e.what() << endl;
        return nullptr;
    }
}

int BuddyAllocator::free(char *_a)
{
    // Try catch block to catch stupid exceptions
    // Note: This does not catch BAD_ACCESS and Segmentation Faults
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

        // while loop to merge blocks over and over again
        while (true)
        {
            // If the buddy is not free or it is already splitted, you're done
            if (!toDeleteBuddy->free or (toDelete->blockSize != toDeleteBuddy->blockSize))
            {
                return 0;
            }

            // Merge the two blocks
            // They will get removed from FreeList by merge
            BlockHeader *mergedResult = merge(toDelete, toDeleteBuddy);
            // Add the merged block back to the FreeList
            FreeList [levelInFreeList + 1].insert(mergedResult);
            
            if (mergedResult->blockSize == total_memory_size) // reached highest level
                return 0;
            
            // Get ready for next iteration
            toDelete = mergedResult;
            toDeleteBuddy = getbuddy(toDelete);
            ++levelInFreeList;
        }
    }
    catch (const std::exception& e)
    {
        cout << "Exception Occured in Free: " << e.what() <<  endl;
        return -1;
    }
    
    return 0;
}

// Given function for printing the freelist
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

// Helper function to calculate index needed in FreeList
int BuddyAllocator::calculateFreeListIndex (int memorySize)
{
    // If memory is bigger than total memory, CAN'T DO IT
    if (memorySize > total_memory_size)
        return -1;
    
    if (memorySize < basic_block_size)
        return 0;
    
    // equation holds true for index > 0
    return ceil (log(memorySize)/log(2) - log(basic_block_size) / log(2));
    
}

// Helper function to return the address of a given block's buddy
BlockHeader * BuddyAllocator::getbuddy(BlockHeader *block)
{
    // No buddy if block is biggest size possible
    if (block->blockSize == total_memory_size)
        return nullptr;
    
    // save address of block and start of allocated memory in 'uintptr_t's
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
    
    return block1;
}

// Helper function to split a given block if it is empty
BlockHeader * BuddyAllocator::split(BlockHeader *block)
{
    // Make sure the given block is free
    // If not, can't split it (IDEALLY this should never happen)
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
