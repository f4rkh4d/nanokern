#pragma once
#include "types.h"

void kbd_push(u8 sc);
int  kbd_read(char *out);
u8   kbd_scancode(void);
