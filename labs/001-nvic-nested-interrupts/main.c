/*
 * Embedded Systems World — Practice Lab 001
 *
 * Does a higher-priority IRQ preempt an ISR on ARM Cortex-M3?
 *
 * QEMU machine: lm3s6965evb. Only NVIC registers are used.
 * Both interrupts are triggered in SOFTWARE via NVIC ISPR.
 * No actual GPIO/UART hardware is needed.
 */
#include <stdint.h>

#define NVIC_ISER0 (*(volatile uint32_t *)0xE000E100UL)
#define NVIC_ISPR0 (*(volatile uint32_t *)0xE000E200UL)
#define NVIC_IPR0  (*(volatile uint8_t  *)0xE000E400UL)
#define NVIC_IPR1  (*(volatile uint8_t  *)0xE000E401UL)

#define IRQ0_BIT  (1UL << 0)
#define IRQ1_BIT  (1UL << 1)

#ifndef SAME_PRIORITY
#define SAME_PRIORITY 0
#endif

static volatile uint32_t trace_count;
static volatile char trace[16];

/* A short log to inspect ONLY after the ISRs complete. No I/O in an ISR. */
static void record(char event)
{
    if (trace_count < sizeof(trace) - 1u) {
        trace[trace_count++] = event;
    }
}

/* ARM semihosting: print NUL-terminated text in QEMU's host terminal. */
static void semihost_print(const char *s)
{
    register uint32_t operation __asm("r0") = 0x04u; /* SYS_WRITE0 */
    register const char *argument __asm("r1") = s;
    __asm volatile("bkpt #0xab" : "+r"(operation) : "r"(argument) : "memory");
}

/* ARM semihosting v2: exit QEMU; SYS_EXIT_EXTENDED takes status code. */
__attribute__((noreturn))
static void semihost_exit(uint32_t status)
{
    const uint32_t parameters[2] = { 0x20026u, status }; /* ADP_Stopped_ApplicationExit */
    register uint32_t operation __asm("r0") = 0x20u; /* SYS_EXIT_EXTENDED */
    register const uint32_t *argument __asm("r1") = parameters;
    __asm volatile("bkpt #0xab" : "+r"(operation) : "r"(argument) : "memory");
    while (1) { }
}

/* IRQ0 is always entered first. */
void IRQ0_Handler(void)
{
    record('A');       /* Entered IRQ0 */
    NVIC_ISPR0 = IRQ1_BIT; /* Pend IRQ1 WHILE IRQ0 is executing */
    record('a');       /* IRQ0 resumed/finished */
}

void IRQ1_Handler(void)
{
    record('B');       /* Entered IRQ1 */
    record('b');       /* Finished IRQ1 */
}

int main(void)
{
    record('M');         /* Main thread running */

    /*
     * Cortex-M uses LOWER numerical priority values for HIGHER urgency.
     * Valid priority bits depend on the implemented NVIC; 0x40 and 0x80
     * are distinct on this emulated Cortex-M3.
     */
    NVIC_IPR0 = 0x80u;  /* IRQ0 priority: lower urgency */
#if SAME_PRIORITY
    NVIC_IPR1 = 0x80u;  /* Same priority: IRQ1 must WAIT */
#else
    NVIC_IPR1 = 0x40u;  /* Higher priority: IRQ1 can PREEMPT */
#endif
    NVIC_ISER0 = IRQ0_BIT | IRQ1_BIT; /* Enable both external interrupts */
    NVIC_ISPR0 = IRQ0_BIT; /* Trigger IRQ0 by software */

    record('m');         /* Main resumes after both IRQs */
    trace[trace_count] = '\0';

#if SAME_PRIORITY
    semihost_print("Case: same priority (IRQ1 waits)\n");
#else
    semihost_print("Case: IRQ1 has higher priority (IRQ1 preempts)\n");
#endif
    semihost_print("Trace: ");
    semihost_print((const char *)(const void *)trace);
    semihost_print("\n");
#if SAME_PRIORITY
    const char *expected = "MAaBbm";
#else
    const char *expected = "MABbam";
#endif
    uint32_t ok = 1u;
    for (uint32_t i = 0; expected[i] != '\0' || trace[i] != '\0'; i++) {
        if (expected[i] != trace[i]) { ok = 0u; break; }
    }
    semihost_print(ok ? "PASS: expected execution order\n" :
                       "FAIL: unexpected interrupt order\n");
    semihost_exit(ok ? 0u : 1u);
}
