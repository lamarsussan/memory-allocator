//
// Created by Lamar on 13/01/2026.
//

#include <unistd.h>
#define MAX_SIZE 100000000

void *smalloc(size_t size){
    if (size == 0 || size > MAX_SIZE){
        return NULL;
    }

    void *first_byte_ptr = sbrk(size);

    if (first_byte_ptr == (void*)(-1)) {
        return NULL;
    }

    return first_byte_ptr;
}