#include "types.h"
#include "heap.h"

// dumb bump allocator. no free. 1 MiB fixed pool.
#define HEAP_SIZE (1 << 20)

static u8 heap[HEAP_SIZE] __attribute__((aligned(16)));
static usize off = 0;

void *kmalloc(usize n) {
    usize aligned = (n + 15) & ~(usize)15;
    if (off + aligned > HEAP_SIZE) return NULL;
    void *p = &heap[off];
    off += aligned;
    return p;
}

usize kheap_used(void) { return off; }
usize kheap_size(void) { return HEAP_SIZE; }
