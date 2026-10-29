//
// Created by Lamar on 13/01/2026.
//
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>

#define MAX_SIZE 100000000
#define MAX_ORDER 10
#define MIN_BLOCK_SIZE 128
#define MAX_BLOCK_SIZE (128 * 1024)
#define INITIAL_BLOCKS 32
#define INITIAL_POOL_SIZE (INITIAL_BLOCKS * MAX_BLOCK_SIZE)

static void* heap_start = nullptr;

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};
// arr of lists in 11 different sizes;
MallocMetadata* free_bins[MAX_ORDER + 1] = {NULL};
bool is_heap_initialized = false;

//globar counters ////////////////////////////////////////
size_t free_blocks_count = 0;
size_t free_bytes_count = 0;
size_t allocated_blocks_count = 0;
size_t allocated_bytes_count = 0;
//////////////////////////////////////////////////////////

int find_order(size_t size) {
    size_t total_size = size + sizeof(MallocMetadata);
    size_t current = MIN_BLOCK_SIZE;
    for (int i = 0; i <= MAX_ORDER; i++) {
        if (current >= total_size) {
            return i;
        }
        current *= 2;
    }
    return -1; // too big //not found
}

//add to bin while keeping the sorting by addr
void add_to_free_bin(MallocMetadata* block, int order) {
    block->is_free = true;
    MallocMetadata* curr = free_bins[order];

    //if list is empty || adrr is the lowest
    if (curr == NULL || block < curr) {
        block->next = curr;
        if (curr != NULL)
            curr->prev = block;
        free_bins[order] = block;
        block->prev = NULL;
    }
    else //find the correct  place to insert
    {
        while (curr->next != NULL && curr->next < block) {
            curr = curr->next;
        }
        block->next = curr->next;
        if (curr->next) curr->next->prev = block;
        curr->next = block;
        block->prev = curr;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

void init_blocks()
{
    intptr_t pb = (intptr_t)sbrk(0);   //currrent program break
    intptr_t new_pb = (INITIAL_POOL_SIZE - (pb % INITIAL_POOL_SIZE)) % INITIAL_POOL_SIZE;
    if(new_pb > 0)
    {
        if (sbrk(new_pb) == (void*)-1) return;
    }

    void* pool_start = sbrk(INITIAL_POOL_SIZE);
    if (pool_start == (void*)-1) return;

    heap_start = pool_start;

    char* index;
   // size_t data_size;
    for(int i = 0; i < INITIAL_BLOCKS; i++)
    {
        index = (char*)pool_start + (i * MAX_BLOCK_SIZE);
        MallocMetadata* block = (MallocMetadata*)(index);
        block->is_free = true;
        block->size = MAX_BLOCK_SIZE;
        block->next = nullptr;
        block->prev = nullptr;
        add_to_free_bin(block, MAX_ORDER); //add to list in size 10
    //  data_size = MAX_BLOCK_SIZE - sizeof(MallocMetadata);
    }
    allocated_blocks_count = INITIAL_BLOCKS;
    allocated_bytes_count = INITIAL_BLOCKS * (MAX_BLOCK_SIZE - sizeof(MallocMetadata));
    free_blocks_count = INITIAL_BLOCKS;
    free_bytes_count = allocated_bytes_count;
    is_heap_initialized = true;
}

//helper func to remove from the linked list
void remove_from_free_bins(int order, MallocMetadata* block) {
    if (free_bins[order] == block) {
        free_bins[order] = block->next;
        if (free_bins[order] != nullptr) {
            free_bins[order]->prev = nullptr;
        }
    }
    else {
        if (block->next != nullptr) {
            block->next->prev = block->prev;
        }
        if (block->prev != nullptr) {
            block->prev->next = block->next;
        }
    }
    //remove the block we used - not exactly
    block->next = nullptr;
    block->prev = nullptr;
}


MallocMetadata* find_free_block(int target_order) {
    for (int i = target_order; i <= MAX_ORDER; ++i) {
        if (free_bins[i] != nullptr) {
            MallocMetadata* block = free_bins[i]; // take first (lowest addr)
            remove_from_free_bins(i, block);
            free_blocks_count--;
            free_bytes_count -= (block->size - sizeof(MallocMetadata));            return block;
        }
    }
    return nullptr; // No block found
}

//challenge 1
// cut block until it fits the user size
void split_block(MallocMetadata* block, int target_order) {
    for (int curr_order = find_order(block->size - sizeof(MallocMetadata));
         curr_order > target_order;
         curr_order--)
    {
        size_t buddy_size = (block->size) / 2;
        //the addr of the second half (which we will not use)
        char* buddy_start_index = (char*)block + buddy_size;
        MallocMetadata* buddy = (MallocMetadata*)buddy_start_index;

        block->size = buddy_size; //the new size of the fist half
        buddy->size = buddy_size;//the new size of the second half
        buddy->is_free = true; //the second half we will not use so its free

        add_to_free_bin(buddy, curr_order - 1);

        allocated_blocks_count++;
        free_blocks_count++;
        allocated_bytes_count -= sizeof(MallocMetadata);
        free_bytes_count += (buddy->size - sizeof(MallocMetadata));

    }
}

//challenge 2
//merge with neighbor if free
MallocMetadata* merge_blocks(MallocMetadata* block) {
    while (block->size < MAX_BLOCK_SIZE) {

        uintptr_t block_addr = (uintptr_t)block;
        uintptr_t relative_addr = block_addr - (uintptr_t)heap_start;
        uintptr_t buddy_relative_addr = relative_addr ^ block->size; //the xor trick to find buddy addr

        if (buddy_relative_addr >= INITIAL_POOL_SIZE) {
            break;
        }

        MallocMetadata* buddy = (MallocMetadata*)((uintptr_t)heap_start + buddy_relative_addr);
        // only merge if buddy is free and the exact same size
        if (!buddy->is_free || buddy->size != block->size) {
            break;
        }
        int curr_order = find_order(buddy->size - sizeof (MallocMetadata));
        free_blocks_count--; //1 less free block after merge
        //free_bytes_count -= (buddy->size - sizeof(MallocMetadata));

        remove_from_free_bins(curr_order, buddy);
        buddy->is_free = false; // no longer separate free

        if(buddy < block) //keep the losest adrr the base of mergedblock
            block = buddy;

        block->size *= 2;
        //one metadata struct is removed
        allocated_blocks_count--;
        allocated_bytes_count += sizeof(MallocMetadata);
        free_bytes_count += sizeof(MallocMetadata);

    }
    return block;
}


void *smalloc(size_t size){

    if (is_heap_initialized == false) init_blocks();

    if (size == 0 || size > MAX_SIZE){
        return nullptr;
    }

    size_t total_size = size + sizeof(MallocMetadata);
    if (total_size > MAX_BLOCK_SIZE) {      //challenge 3

        void* map = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                         MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

        if (map == MAP_FAILED){
            return nullptr;
        }

        MallocMetadata* block = (MallocMetadata*)map;
        block->size = total_size;
        block->is_free = false;
        block->next = nullptr;
        block->prev = nullptr;

        allocated_blocks_count++;
        allocated_bytes_count += size;
        return (void*)(block + 1);

    }

//normal buddy system allocation
    int target_order = find_order(size);
    if(target_order == -1 ){
        return nullptr;
    }
    MallocMetadata* block = find_free_block(target_order);

    if (block == nullptr) {
        return nullptr;
    }

    split_block(block, target_order); //challenge 1
    block->is_free = false;
   // allocated_bytes_count += (block->size - sizeof(MallocMetadata));
    return (void*)(block + 1);    //pointer to data payload in the block
}

//allocate and zero the mem
void* scalloc(size_t num, size_t size){
    if(size == 0 || num ==0 || size*num > MAX_SIZE){
        return nullptr;
    }

    void* pointer = smalloc(size*num);
    if (pointer != nullptr){
        memset(pointer,0,size*num);
    }
    return pointer;
}

//free and use merge func
void sfree(void* p){
    if(p == nullptr || is_heap_initialized == false){
        return;
    }
    //because metadata is located before the data pointer
    MallocMetadata* block = (MallocMetadata*)p - 1;

    if(block->is_free){
        return;
    }

    if (block->size > MAX_BLOCK_SIZE) { //challenge 3
        allocated_blocks_count--;
        allocated_bytes_count -= (block->size - sizeof(MallocMetadata));
        munmap(block, block->size);
        return;
    }
    //normal free
    block->is_free = true;
   // allocated_bytes_count -= (block->size - sizeof(MallocMetadata));
    free_blocks_count++;
    free_bytes_count += (block->size - sizeof(MallocMetadata));


    MallocMetadata* final_block = merge_blocks(block); //challenge 2
    int order = find_order(final_block->size - sizeof(MallocMetadata));
    add_to_free_bin(final_block, order);  // put final merged block back into the correct bin
}

void* srealloc(void* oldp, size_t size) {
    if (!is_heap_initialized) init_blocks();
    if (size == 0 || size > MAX_SIZE){
        return nullptr;
    }
    if (!oldp) {
        return smalloc(size);
    }

    MallocMetadata* metadata = (MallocMetadata*)oldp - 1;
    size_t old_payload_size = metadata->size - sizeof(MallocMetadata);

    if (metadata->size > MAX_BLOCK_SIZE) {
        if (old_payload_size == size) return oldp;
        void* newp = smalloc(size);
        if (!newp) return nullptr;
        memcpy(newp, oldp, (old_payload_size < size) ? old_payload_size : size);
        sfree(oldp);
        return newp;
    }

    int target_order = find_order(size);
    int current_order = find_order(old_payload_size);
    //reuse if enough
    if (current_order >= target_order) {
        return oldp;
    }
    //try merge inplace with buddy
    size_t potential_size = metadata->size;
    uintptr_t current_addr = (uintptr_t)metadata;

    while (potential_size < MAX_BLOCK_SIZE) {
        uintptr_t relative_addr = current_addr - (uintptr_t)heap_start;
        uintptr_t buddy_addr = (uintptr_t)heap_start + (relative_addr ^ potential_size);

        MallocMetadata* buddy = (MallocMetadata*)buddy_addr;
        // check if neighbor is free and same size to consider merging
        if (!buddy->is_free || buddy->size != potential_size) break;

        potential_size *= 2;
        if (buddy_addr < current_addr) current_addr = buddy_addr;

        if (potential_size - sizeof(MallocMetadata) >= size) { //merge and update counts
            uintptr_t temp_addr = (uintptr_t)metadata;
            size_t temp_size = metadata->size;

            while (temp_size < potential_size) {
                uintptr_t b_addr = (uintptr_t)heap_start + (((uintptr_t)temp_addr - (uintptr_t)heap_start) ^ temp_size);
                MallocMetadata* b_ptr = (MallocMetadata*)b_addr;

                remove_from_free_bins(find_order(b_ptr->size - sizeof(MallocMetadata)), b_ptr);
                //buddy is no longer free or separate block
                free_blocks_count--;
                free_bytes_count -= (b_ptr->size - sizeof(MallocMetadata));
                allocated_blocks_count--;

                allocated_bytes_count += sizeof(MallocMetadata);

                if (b_addr < temp_addr) temp_addr = b_addr;
                temp_size *= 2;
            }

            MallocMetadata* merged = (MallocMetadata*)temp_addr;
            merged->size = temp_size;
            merged->is_free = false;
            //split the merged block if it is too big now
            split_block(merged, target_order);
            // if address changed, move data safely with memmove
            if ((void*)(merged + 1) != oldp) {
                memmove(merged + 1, oldp, old_payload_size);
            }
            return (void*)(merged + 1);
        }
    }
//else find new block
    void* newp = smalloc(size);
    if (!newp) return nullptr;
    memcpy(newp, oldp, old_payload_size);
    sfree(oldp);// sfree will handle merging the old block with its buddies
    return newp;
}

size_t _num_free_blocks(){
    return free_blocks_count;
}
size_t _num_free_bytes(){
    return free_bytes_count;
}
size_t _num_allocated_blocks(){
    return allocated_blocks_count;
}
size_t _num_allocated_bytes(){
    return allocated_bytes_count;
}
size_t _num_meta_data_bytes(){
    return allocated_blocks_count * sizeof(MallocMetadata) ;
}
size_t _size_meta_data(){
    return sizeof(MallocMetadata);
}