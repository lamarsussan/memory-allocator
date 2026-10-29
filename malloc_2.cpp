//
// Created by Lamar on 13/01/2026.
//
#include <unistd.h>
#include <string.h>
#define MAX_SIZE 100000000

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

// global pointers to manage the memory list
MallocMetadata* head = NULL;
MallocMetadata* tail = NULL;

size_t free_blocks_count = 0;
size_t free_bytes_count = 0;
size_t allocated_blocks_count = 0;
size_t allocated_bytes_count = 0;


void *smalloc(size_t size){
    if (size == 0 || size > MAX_SIZE){
        return NULL;
    }
    //search for the first available block (First-fit)
    // if(size > free_bytes_count) return NULL;
    MallocMetadata* current = head;
    while (current){
        if(current->is_free == true && current->size >= size){
            current->is_free = false;
            free_blocks_count -= 1;
            free_bytes_count -= current ->size;
            return (void *)(current+1); //+1 for excluding the meatadata
        }
        current = current->next;
    }
    //No existing block fits so we request more memory from OS
    size_t total_required = size + sizeof(MallocMetadata);
    void* ptr = sbrk(total_required);
//if the sbrk fails
    if (ptr ==  (void *)(-1)) {
        return NULL;
    }

    MallocMetadata* metadata = (MallocMetadata*) ptr;
    metadata->size = size;
    metadata->is_free = false;
    metadata->next = NULL;
    metadata->prev = tail;

    //Update the doubly linked list pointers
    if (head == NULL) { //if this is the first block in the list
        head = metadata;
    } else { //add to the end of the list
        tail->next = metadata;
    }
    tail = metadata;

    // update global counters
    allocated_blocks_count++;
    allocated_bytes_count += size;

    return (void*)(metadata + 1);
}

void* scalloc(size_t num, size_t size){
    if(size == 0 || num ==0 || size*num > MAX_SIZE){
        return NULL;
    }

    void* pointer = smalloc(size*num);
    if (pointer != NULL){
        memset(pointer,0,size*num);
    }
    return pointer;
}

void sfree(void* p){
    if(p == NULL){
        return;
    }
    //because metadata is located before the data pointer
    MallocMetadata* metadata = (MallocMetadata*)p - 1;

    if(metadata->is_free){
        return;
    }
    metadata->is_free = true;
    free_blocks_count +=1;
    free_bytes_count += metadata->size;

}

void* srealloc(void* oldp, size_t size){
    if (size == 0 || size > MAX_SIZE) {
        return NULL;
    }
    if (oldp == NULL) {
        return smalloc(size);
    }

    MallocMetadata* metadata = (MallocMetadata*)oldp - 1;
    if(metadata->size >= size){ //if current block is large enough so reuse it
        return oldp;
    }
    else{
        void* newp = smalloc(size);
        if (newp == NULL){
            return NULL;
        }
        memmove(newp,oldp,metadata->size);
        sfree(oldp);
        return newp;
    }
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

