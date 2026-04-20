#pragma once
#include "types.h"

void *kmalloc(usize n);
usize kheap_used(void);
usize kheap_size(void);
