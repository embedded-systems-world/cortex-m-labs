/*
 * Bare-metal ARMv7-M (Cortex-M3) startup for QEMU lm3s6965evb.
 * This is an educational example: not production MCU startup code.
 */
#include <stdint.h>

extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss;
int main(void);
void Reset_Handler(void);
void Default_Handler(void);
void IRQ0_Handler(void);
void IRQ1_Handler(void);

void Reset_Handler(void)
{
    /* Copy initialized data from flash to RAM. */
    for (uint32_t *dst = &_sdata, *src = &_sidata; dst < &_edata; ) {
        *dst++ = *src++;
    }
    /* Zero uninitialized globals. */
    for (uint32_t *dst = &_sbss; dst < &_ebss; ) {
        *dst++ = 0u;
    }
    (void)main();
    while (1) { /* main normally exits via QEMU semihosting */ }
}

void Default_Handler(void)
{
    while (1) { /* Attach debugger to inspect an unexpected exception */ }
}

__attribute__((used, section(".isr_vector")))
const uintptr_t vector_table[] = {
    (uintptr_t)&_estack,          /* 0: initial MSP */
    (uintptr_t)Reset_Handler,    /* 1: Reset */
    (uintptr_t)Default_Handler,  /* 2: NMI */
    (uintptr_t)Default_Handler,  /* 3: HardFault */
    (uintptr_t)Default_Handler,  /* 4: MemManage */
    (uintptr_t)Default_Handler,  /* 5: BusFault */
    (uintptr_t)Default_Handler,  /* 6: UsageFault */
    0, 0, 0, 0,
    (uintptr_t)Default_Handler,  /* 11: SVCall */
    (uintptr_t)Default_Handler,  /* 12: DebugMon */
    0,
    (uintptr_t)Default_Handler,  /* 14: PendSV */
    (uintptr_t)Default_Handler,  /* 15: SysTick */
    (uintptr_t)IRQ0_Handler,     /* 16: External IRQ0 */
    (uintptr_t)IRQ1_Handler,     /* 17: External IRQ1 */
};
