#include <iostream>
#include <cstddef>

// Functions implemented in malloc_3.cpp
void* smalloc(size_t size);
void sfree(void* p);
size_t _num_free_blocks();
size_t _num_free_bytes();

void show_stats(const char* stage) {
    std::cout << "\n[" << stage << "]\n";
    std::cout << "Free blocks: " << _num_free_blocks() << '\n';
    std::cout << "Free bytes:  " << _num_free_bytes() << '\n';
}

int main() {
    std::cout << "=== Memory Allocator Demo ===\n";

    void* first = smalloc(100);
    if (!first) {
        std::cerr << "Allocation failed\n";
        return 1;
    }
    std::cout << "Allocated 100 bytes\n";
    show_stats("After allocation");

    sfree(first);
    std::cout << "\nFreed the allocation\n";
    show_stats("After free");

    void* second = smalloc(100);
    if (!second) {
        std::cerr << "Second allocation failed\n";
        return 1;
    }
    std::cout << "\nAllocated another 100 bytes\n";
    std::cout << "Reused the same address: "
              << (second == first ? "yes" : "no") << '\n';
    show_stats("After second allocation");

    sfree(second);
    std::cout << "\nDemo complete\n";
    return 0;
}
