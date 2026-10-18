# Lab 002 — Tail-Chaining and Exception Return on Cortex-M3

**Embedded Systems World · Why Series companion lab · Learn → Experiment → Understand**

> **Validation in progress:** This lab is a draft until its QEMU CI job passes. Do not cite it as measured proof of tail-chaining cycles.

## Learning goal

When one IRQ becomes pending during another ISR, what happens to the saved Thread-mode context? Contrast a **pending and eligible** interrupt with a **pending but NVIC-disabled** interrupt.

The firmware runs on QEMU's Cortex-M3 `lm3s6965evb` virtual board. It software-pends external IRQ0 ("Timer-like") and IRQ1 ("UART-like"). No real timer/UART peripheral is modeled and no RTOS is required.

## Get started (Ubuntu or WSL)

```bash
sudo apt update
sudo apt install qemu-system-arm gcc-arm-none-eabi make python3
cd labs/002-tail-chaining-exception-return
make test
```

Or execute the cases separately:

```bash
make run-tail
make run-deferred
```

## Two experiments

**A — UART-like IRQ enabled and eligible when Timer-like IRQ returns**

IRQ0 priority byte `0x40` (higher urgency) and IRQ1 priority byte `0x80` (lower urgency). While IRQ0 is active, it software-pends IRQ1. IRQ1 cannot preempt IRQ0 but may execute on its return to Thread mode.

**Expected trace:** `MTtUum`

```text
main → Timer ISR → UART ISR → main
```

**B — UART-like IRQ pending but disabled**

IRQ1 becomes pending in IRQ0, but the NVIC enable bit is clear. IRQ0 returns to `main()`. Thread mode records `m`, enables IRQ1, and IRQ1 then executes.

**Expected trace:** `MTtmUun`

```text
main → Timer ISR → main (enables IRQ1) → UART ISR → main
```

Legend: `M` initial main; `T/t` Timer handler entry/end; `U/u` UART handler entry/end; `m` first Thread-mode continuation; `n` final Thread-mode continuation in Experiment B.

## The saved exception frame

For Cortex-M3's basic hardware exception frame, with the stack pointer at the bottom of the frame:

```text
MSP+0x00  R0
MSP+0x04  R1
MSP+0x08  R2
MSP+0x0C  R3
MSP+0x10  R12
MSP+0x14  LR
MSP+0x18  PC      <- interrupted program's resume address
MSP+0x1C  xPSR
```

The handler's **live LR** contains an `EXC_RETURN` value, here expected to be `0xFFFFFFF9`: return to Thread mode using MSP. This is distinct from the **stacked LR** from the interrupted program.

The `naked` entry wrappers capture MSP and the live exception-return LR before an ordinary C function prologue moves SP. The snapshots also record stacked PC, stacked xPSR and IPSR (16 for external IRQ0; 17 for external IRQ1).

In case A, saved PC values should match because main did not execute between handlers. In case B, saved PC values should differ because main resumed and advanced before the second IRQ.

**Important:** The same MSP address may appear even when the processor restored and later restacked a frame at the same memory address; an equal address is not proof that no restack happened.

## Inspect with GDB

Terminal 1:

```bash
make debug-tail
```

Terminal 2 (requires gdb-multiarch or Arm GDB):

```bash
gdb-multiarch build/tail.elf
```

In GDB:

```gdb
set pagination off
target remote :1234
break timer_isr_body
break uart_isr_body
continue
info registers lr sp pc
print/x timer_snapshot
continue
info registers lr sp pc
print/x uart_snapshot
```

Note: breakpoints can perturb timing.

## What the lab establishes — and what it does not

The event trace and exception frame snapshots are consistent with Cortex-M tail-chaining semantics. **They do not directly prove a particular number of cycles saved or count hardware memory accesses.** QEMU is not a cycle-accurate replacement for physical Cortex-M silicon. To measure exact exception latency, use suitable Cortex-M hardware and a cycle counter or trace mechanism while accounting for execution and memory-system overhead.

## Try changing the firmware

1. Change IRQ1 to the same priority as IRQ0. Does it preempt? What happens when IRQ0 completes?
2. Leave IRQ1 disabled longer. What happens to its pending state?
3. Compare live LR (`EXC_RETURN`) with the stacked LR field in the frame.
4. Explain why identical saved PC and MSP values are suggestive but not sufficient to measure unstack/restack savings.

[Embedded Systems World](https://embeddedsysworld.com/) · Free Cortex-M practice lab
