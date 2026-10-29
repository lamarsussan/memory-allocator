# Memory Allocator

Dynamic memory allocator implemented in C++ on Linux.

## Problem

Implement dynamic memory allocation functionality without using the standard `malloc`, `free`, `calloc`, or `realloc` functions.

## Implementation

- Metadata-based block management
- Reuse of freed blocks
- Buddy allocator with block splitting and merging
- Heap allocation using `sbrk`
- Large allocations using `mmap` and `munmap`

## Setup

Implemented and tested on Ubuntu 18.04 using `g++`.

## Tests

Tested using the provided course Makefile and automated test targets (`make test1`, `make test2`, `make test3`).
