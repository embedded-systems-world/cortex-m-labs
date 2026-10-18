# Lab 001 — NVIC Interrupt Priority and Preemption

**Embedded Systems World · Cortex-M learning track**

This lab demonstrates the distinction between **higher-priority ISR preemption** and **equal-priority pending interrupts**. Run bare-metal ARM Cortex-M3 firmware on the simulated `lm3s6965evb` board in QEMU. Both external IRQs are triggered using NVIC software pending bits (no physical timer or UART peripheral).

## Run on Ubuntu/WSL

```bash
sudo apt update
sudo apt install qemu-system-arm gcc-arm-none-eabi make python3
make test
```

Expected output traces:

| Scenario | Trace | Interpretation |
| --- | --- | --- |
| IRQ1 higher urgency | `MABbam` | IRQ1 preempts IRQ0, then IRQ0 resumes |
| IRQ1 same priority | `MAaBbm` | IRQ1 waits until IRQ0 finishes |

`M/m`: main; `A/a`: IRQ0; `B/b`: IRQ1.

Explore the priority assignments and NVIC software-pending writes in `main.c`. Change priority values, predict the sequence, and run again.

**Limitations:** The trace confirms execution order in QEMU, not cycle-accurate proof of tail-chaining. NVIC priority bits depend on MCU implementation. Only use semihosting with trusted code.

[Embedded Systems World](https://embeddedsysworld.com/) · Learn → Experiment → Understand
