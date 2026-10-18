CC := arm-none-eabi-gcc
QEMU ?= qemu-system-arm
CPU_FLAGS := -mcpu=cortex-m3 -mthumb
CFLAGS := $(CPU_FLAGS) -std=c11 -Os -g3 -Wall -Wextra -Werror -ffreestanding -fno-builtin -ffunction-sections -fdata-sections
LDFLAGS := $(CPU_FLAGS) -nostdlib -Wl,-T,linker.ld -Wl,--gc-sections -Wl,-Map,build/lab.map
SOURCES := startup.c main.c
QEMU_FLAGS := -M lm3s6965evb -nographic -monitor none -semihosting-config enable=on,target=native

.PHONY: all run-tail run-deferred test clean debug-tail
all: build/tail.elf build/deferred.elf

build:
	mkdir -p build

build/tail.elf: $(SOURCES) linker.ld | build
	$(CC) $(CFLAGS) -DDEFER_SECOND=0 $(SOURCES) $(LDFLAGS) -o $@

build/deferred.elf: $(SOURCES) linker.ld | build
	$(CC) $(CFLAGS) -DDEFER_SECOND=1 $(SOURCES) $(LDFLAGS) -o $@

run-tail: build/tail.elf
	$(QEMU) $(QEMU_FLAGS) -kernel $<

run-deferred: build/deferred.elf
	$(QEMU) $(QEMU_FLAGS) -kernel $<

test: all
	python3 test_trace.py

debug-tail: build/tail.elf
	$(QEMU) $(QEMU_FLAGS) -S -gdb tcp::1234 -kernel $<

clean:
	rm -rf build
