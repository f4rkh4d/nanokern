CC      := x86_64-elf-gcc
LD      := x86_64-elf-ld
OBJCOPY := x86_64-elf-objcopy
NASM    := nasm
QEMU    := qemu-system-x86_64

BUILD      := build
KERNEL64   := $(BUILD)/nanokern.elf64
KERNEL     := $(BUILD)/nanokern.elf

CFLAGS  := -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone \
           -mno-mmx -mno-sse -mno-sse2 -mcmodel=kernel \
           -Wall -Wextra -std=c11 -O2 -g -nostdlib \
           -Isrc/kernel

LDFLAGS := -n -T linker.ld -nostdlib -z max-page-size=0x1000

ASM_SRC := src/boot/boot.s src/kernel/arch/isr_stubs.s
C_SRC   := $(wildcard src/kernel/*.c)

ASM_OBJ := $(patsubst %.s,$(BUILD)/%.o,$(ASM_SRC))
C_OBJ   := $(patsubst %.c,$(BUILD)/%.o,$(C_SRC))
OBJ     := $(ASM_OBJ) $(C_OBJ)

.PHONY: all run clean

all: $(KERNEL)

$(KERNEL64): $(OBJ) linker.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

# qemu's -kernel multiboot loader only accepts elf32 images. we link 64-bit code and then
# repackage the elf container as elf32; the 64-bit instruction bytes survive unchanged and
# execute correctly once boot.s has flipped the cpu into long mode.
$(KERNEL): $(KERNEL64)
	$(OBJCOPY) -O elf32-i386 $< $@

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.s
	@mkdir -p $(dir $@)
	$(NASM) -f elf64 $< -o $@

run: $(KERNEL)
	$(QEMU) -kernel $(KERNEL) -serial stdio -m 128M -no-reboot -no-shutdown

run-nox: $(KERNEL)
	$(QEMU) -kernel $(KERNEL) -serial stdio -display none -m 128M -no-reboot

clean:
	rm -rf $(BUILD)
