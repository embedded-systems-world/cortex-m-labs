# Build with GNU Arm Embedded toolchain; run with QEMU system emulator.
CC ?= arm-none-eabi-gcc
# Built-in Make CC=cc overrides ?=; set explicitly if environment not configured:
CC := arm-none-eabi-gcc
QEMU ?= qemu-system-arm

CPU_FLAGS = -mcpu=cortex-m3 -mthumb
CFLAGS = $(CPU_FLAGS) -std=c11 -Os -Wall -Wextra -Werror -ffreestanding -fno-builtin -fdata-sections -ffunction-sections
LDFLAGS = $(CPU_FLAGS) -nostdlib -Wl,-T,linker.ld -Wl,--gc-sections -Wl,-Map,build/lab.map
SOURCES = startup.c main.c
QEMU_FLAGS = -M lm3s6965evb -nographic -monitor none -semihosting-config enable=on,target=native

.PHONY: all clean run run-equal test
all: build/preempt.elf build/equal.elf

build:
	mkdir -p build

build/preempt.elf: $(SOURCES) linker.ld | build
	$(CC) $(CFLAGS) -DSAME_PRIORITY=0 $(SOURCES) $(LDFLAGS) -o $@

build/equal.elf: $(SOURCES) linker.ld | build
	$(CC) $(CFLAGS) -DSAME_PRIORITY=1 $(SOURCES) $(LDFLAGS) -o $@

run: build/preempt.elf
	$(QEMU) $(QEMU_FLAGS) -kernel $<

run-equal: build/equal.elf
	$(QEMU) $(QEMU_FLAGS) -kernel $<

test: all
	python3 test_trace.py

clean:
	rm -rf build
