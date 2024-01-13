#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct heap_block_header heap_block;

struct heap_block_header
{
    heap_block* next;
    heap_block* prev;
    void* start;
    uint32_t length;
    bool in_use;
};

typedef struct heap_header heap;

struct heap_header
{
    heap_block* end;
    void* addr;
    uint32_t len;
    heap_block* start;
};

// CLIENT INTERFACE START

typedef heap* heap_t;

static heap_t H;

void heap_init(uint32_t memlength, void* start_addr, void* heap_addr);
void* malloc(uint32_t size);
void free(void* p);

// CLIENT INTERFACE END

// DEBUG START
void heap_print(heap_t H); 
void memprint(heap_t H);
// DEBUG END