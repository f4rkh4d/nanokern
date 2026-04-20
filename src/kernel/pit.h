#pragma once
#include "types.h"

extern volatile u64 pit_ticks;
void pit_init(u32 hz);
