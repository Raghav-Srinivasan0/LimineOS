#include "heap.h"
#include <assert.h>

void heap_init(uint32_t memlength, void* start_addr, void* heap_addr)
{
    H = (heap*)heap_addr;
    heap_block* first_block = (heap_block*)((char*)heap_addr + sizeof(heap));
    first_block->length = memlength;
    first_block->next = NULL;
    first_block->prev = NULL;
    first_block->start = start_addr;
    first_block->in_use = false;
    H->start = first_block;
    H->addr = start_addr;
    H->len = memlength;
    H->end = first_block;
}

/*
void heap_block_print(heap_block* h)
{
    printf("Start Addr: %p\nLength: %ld\nIn Use: %s\n\n",h->start,h->length,h->in_use ? "True": "False");
}
*/

/*
void heap_print()
{
    heap_block* cur = H->start;
    while (cur != NULL)
    {
        heap_block_print(cur);
        cur = cur->next;
    }
}
*/

void* malloc(uint32_t size)
{
    heap_block* cur = H->start;
    while (cur != NULL)
    {
        if (!(cur->in_use) && (cur->length >= size))
        {
            // if we found an unused block of memory that has the required size
            uint32_t remaining_size = cur->length - size;
            void* remaining_start = (void*)(((char*)cur->start) + size);
            cur->in_use = true;
            cur->length = size;
            heap_block* remaining_block = (heap_block*)((char*)H->end + sizeof(heap_block));
            H->end = remaining_block;
            remaining_block->in_use = false;
            remaining_block->length = remaining_size;
            remaining_block->start = remaining_start;
            remaining_block->prev = cur;
            remaining_block->next = cur->next;
            if (remaining_block->next != NULL)
                remaining_block->next->prev = remaining_block;
            cur->next = remaining_block;
            return cur->start;
        }
        cur = cur->next;
    }
    return NULL; // Couldn't find an unused block with the required conditions
}

void heap_block_free(heap_block* h)
{
    heap_block* next = h->next;
    heap_block* prev = h->prev;
    if (next != NULL)
        next->prev = prev;
    if (prev != NULL)
        prev->next = next;
}

void defragment()
{
    heap_block* cur = H->start;
    while (cur != NULL && cur->next != NULL)
    {
        assert(cur != NULL);
        if (!(cur->in_use || cur->next->in_use))
        {
            // Merge the two blocks
            uint32_t length = cur->next->length;
            heap_block_free(cur->next);
            cur->length += length;
        }
        cur = cur->next;
    }
}

void free(void* p)
{
    heap_block* cur = H->start;
    while (cur != NULL)
    {
        if (cur->start == p) break;
        cur = cur->next;
    }
    if (cur == NULL)
        return;
    cur->in_use = false;
    defragment();
}

/*
void memprint(heap* H)
{
    char* data = (char*)H->addr;
    for (uint32_t i = 0; i<H->len; i++)
    {
        printf("%02x ",data[i] & 0xff);
        if (i % 10 == 9)
            printf("\n");
    }
}
*/