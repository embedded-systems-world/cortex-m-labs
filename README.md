# Embedded Systems World — Cortex-M Practice Labs

**Learn → Experiment → Understand**

Free, reproducible bare-metal ARM Cortex-M examples for the Embedded Systems World learning series.

| Lab | Topic | Goal |
| --- | --- | --- |
| [Lab 001](labs/001-nvic-nested-interrupts/) | NVIC priority and preemption | Verify when an interrupt preempts or waits |
| Lab 002 (in preparation) | Tail-chaining and exception return | Inspect the hardware exception frame and EXC_RETURN |

**Tools:** Linux or WSL, GNU Arm Embedded toolchain, QEMU `qemu-system-arm`, Python 3.

Lab 001:
```bash
sudo apt update
sudo apt install gcc-arm-none-eabi qemu-system-arm make python3
make -C labs/001-nvic-nested-interrupts test
```

The examples use NVIC software-pended interrupts on a simulated Cortex-M3, not real UART or timer peripherals. Execution traces establish order; they are not cycle-accurate measurements of tail-chaining.

[Embedded Systems World](https://embeddedsysworld.com/) · Learn → Experiment → Understand
