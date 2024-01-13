#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

#include "heap.h"
#include "kernel.h"
#include "gdt.h"
#include "idt.h"

const uint64_t memsize = 2000000000;

// Set the base revision to 1, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

LIMINE_BASE_REVISION(1)

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, in C, they should
// NOT be made "static".

struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .c file.

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }

    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    if (src > dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}

// Halt and catch fire function.
static void hcf(void) {
    asm ("cli");
    for (;;) {
        asm ("hlt");
    }
}

static void asrt(bool expr)
{
    if (!expr)
    {
        hcf();
    }
}

// Our quick and dirty strlen() implementation.
size_t strlen(const char *str) {
    size_t ret = 0;
    while (*str++) {
        ret++;
    }
    return ret;
}

void debug_line(struct limine_framebuffer *framebuffer)
{
    // Note: we assume the framebuffer model is RGB with 32-bit pixels.
    for (size_t i = 0; i < 100; i++) {
        volatile uint32_t *fb_ptr = framebuffer->address;
        fb_ptr[i * (framebuffer->pitch / 4) + i] = 0xffffff;
    }

    hcf();
}

// The following will be our kernel's entry point.
// If renaming _start() to something else, make sure to change the
// linker script accordingly.
void _start(void) {

    gdt_init();
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // Fetch the first framebuffer.
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    // Ensure we got a memory map
    if (memmap_request.response == NULL) {
        hcf();
    }

    // Ensure we got a HHDM
    if (hhdm_request.response == NULL) {
        hcf();
    }

    uint64_t num_entries = (memmap_request.response)->entry_count;
    struct limine_memmap_entry **arr_entries = (memmap_request.response)->entries;

    uint64_t max_memory = 0;
    void* addr_max_memory = NULL;

    for (uint64_t i = 0; i<num_entries; i++)
    {
        struct limine_memmap_entry* cur = arr_entries[i];
        if (cur->type == LIMINE_MEMMAP_USABLE && cur->length >= max_memory)
        {
            max_memory = cur->length;
            addr_max_memory = (void*)cur->base;
            break;
        }
    }

    void* hhdm_start = (void*)(hhdm_request.response->offset);
    
    if (addr_max_memory == NULL)
    {
        hcf();
    }

    heap_init(memsize,hhdm_start,hhdm_start+memsize); // Fixed, but hacky. Need to find out how much space the heap needs at its worst and provide only that

    FIS_REG_H2D fis;
    memset(&fis, 0, sizeof(FIS_REG_H2D));
    fis.fis_type = FIS_TYPE_REG_H2D;
    fis.command = ATA_CMD_IDENTIFY;	// 0xEC
    fis.device = 0;			// Master device
    fis.c = 1;				// Write command register

    idt_init();

    debug_line(framebuffer);

    // We're done, just hang...
    hcf();
}
