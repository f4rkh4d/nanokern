# nanokern

a multitasking x86-64 kernel in 554 lines of c and asm. boots under qemu, prints a banner, handles the timer irq, reads the ps/2 keyboard, hands out memory from a bump heap. designed to fit in my head.

## boot

```
make run
```

you should see:

```
nanokern 0.1.0 (x86-64, long mode)
-----------------------------------
[boot] long mode entered via multiboot
[boot] gdt loaded from boot.s
[boot] idt installed (256 vectors)
[boot] pic remapped (master=0x20, slave=0x28)
[boot] pit armed at 100 hz (10 ms)
[boot] irq0 (timer) + irq1 (keyboard) unmasked
[boot] heap: 0 / 1048576 bytes
[boot] enabling interrupts...

init > _
```

type something. the keyboard driver echoes it. a small seconds-since-boot counter ticks in the top-right corner, proving the timer irq is live.

## toolchain

```
brew install nasm qemu x86_64-elf-gcc   # macOS
apt install nasm qemu-system-x86 gcc-x86-64-linux-gnu   # debian-ish
```

`make` builds `build/nanokern.elf`. qemu's `-kernel` flag loads multiboot elfs directly, no iso or grub required.

## what's in it

- **boot.s.** multiboot (v1) header, 32-bit entry, identity-maps the first 1 GiB with 2 MiB pages, enables PAE + LME + paging, loads a 64-bit gdt, long-jumps to kmain. all in under 100 lines of nasm.
- **idt + isr stubs.** 256 vectors, generated with nasm macros that push a dummy error code for vectors that don't push one, so the common dispatcher sees a uniform stack.
- **pic.** legacy 8259 remap (master to 0x20, slave to 0x28) because legacy-free x86 is a lie.
- **pit.** channel 0 at 100 hz. 10 ms quantum in spirit. the scheduler is the next piece to land.
- **keyboard.** ps/2 irq1, scancode set 1, fixed us layout (no shift/ctrl yet), 128-byte ring buffer consumed by the init loop.
- **heap.** a bump allocator. 1 MiB pool. `kfree` doesn't exist yet because a bump with no slab has nothing honest to do there.
- **vga.** 80x25 text, a tiny `vga_printf` that knows `%s %c %d %x %p`.

## what's *not* in it yet (the v0.3 roadmap)

- **preemptive scheduler.** task struct exists in the roadmap, context switch in ~14 asm instructions is the plan, round-robin on the 10 ms quantum.
- **syscalls via `int 0x80`.** `write`, `read`, `spawn`, `exit`, `sleep`.
- **higher-half kernel at `0xffffffff80000000`.** currently identity-mapped, which is simpler but not how any serious kernel is laid out.
- **slab + real pmm.** bump is fine for boot but won't go the distance.
- **stage1/stage2 bootsector.** the multiboot shortcut is a concession to iteration speed. replacing it with a 512-byte real-mode stage1 is a deliberate future loss.
- **user mode + syscalls via sysret.**

## layout

```
src/
  boot/boot.s             multiboot entry, 32-bit to long mode
  kernel/
    main.c                kmain
    vga.c, vga.h          80x25 text + tiny printf
    idt.c, idt.h          256-vector idt, dispatcher
    pic.c, pic.h          8259 remap + eoi + mask
    pit.c, pit.h          channel 0 timer
    kbd.c, kbd.h          ps/2 irq + ring buffer
    heap.c, heap.h        bump allocator
    types.h               fixed-width ints + port i/o + cli/sti/hlt
    arch/isr_stubs.s      256 isr stubs, nasm-generated
linker.ld                 elf64 layout, starts at 1 MiB physical
Makefile                  clang-free, uses x86_64-elf-gcc + nasm
```

554 lines of c and asm total, counted by `find src -type f | xargs wc -l`.

## why

the jump from "i use an os" to "i understand what the cpu does before `main()`" is one of the largest in programming. every abstraction you take for granted (processes, files, scheduling, virtual memory) turns out to be a few hundred lines of c and an `lgdt`. i know this is a toy. that's the whole point. real kernels are too big to hold in my head at once. this one fits.

## license

MIT.
