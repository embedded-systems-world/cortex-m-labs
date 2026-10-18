/*
 * Embedded Systems World — Lab 002
 * Cortex-M3 tail-chaining vs return-to-thread when an IRQ is disabled.
 *
 * External IRQ0/IRQ1 are software-pended via NVIC. They model Timer/UART
 * scheduling only; no physical Timer or UART peripheral is used.
 * Target: QEMU lm3s6965evb (Cortex-M3), default Thread-mode MSP.
 */
#include <stdint.h>

#define NVIC_ISER0 (*(volatile uint32_t *)0xE000E100UL)
#define NVIC_ISPR0 (*(volatile uint32_t *)0xE000E200UL)
#define NVIC_IPR0  (*(volatile uint8_t *)0xE000E400UL)
#define NVIC_IPR1  (*(volatile uint8_t *)0xE000E401UL)
#define IRQ0_BIT   (1UL << 0)
#define IRQ1_BIT   (1UL << 1)

#ifndef DEFER_SECOND
#define DEFER_SECOND 0
#endif

/* Basic exception frame: R0, R1, R2, R3, R12, LR, PC, xPSR. */
#define FRAME_PC_INDEX   6u
#define FRAME_XPSR_INDEX 7u
#define RETURN_TO_THREAD_MSP 0xFFFFFFF9UL

typedef struct {
    volatile uint32_t entry_msp;
    volatile uint32_t exc_return;
    volatile uint32_t stacked_pc;
    volatile uint32_t stacked_xpsr;
    volatile uint32_t ipsr;
} Snapshot;

static Snapshot timer_snapshot;
static Snapshot uart_snapshot;
static volatile char trace[24];
static volatile uint32_t trace_count;

static void record(char event)
{
    if (trace_count < sizeof(trace) - 1u) {
        trace[trace_count++] = event;
    }
}

static uint32_t read_ipsr(void)
{
    uint32_t value;
    __asm volatile ("mrs %0, ipsr" : "=r"(value));
    return value;
}

static void save_snapshot(Snapshot *dst, const uint32_t *frame,
                          uint32_t exc_return)
{
    dst->entry_msp = (uint32_t)(uintptr_t)frame;
    dst->exc_return = exc_return;
    dst->stacked_pc = frame[FRAME_PC_INDEX];
    dst->stacked_xpsr = frame[FRAME_XPSR_INDEX];
    dst->ipsr = read_ipsr();
}

/* Read the genuine exception-entry MSP before C compiler prologue changes it.
 * The naked wrappers branch to ordinary C functions, preserving LR's
 * EXC_RETURN value for the normal hardware exception return. */
__attribute__((used, noinline))
void timer_isr_body(uint32_t *exception_frame, uint32_t exc_return)
{
    save_snapshot(&timer_snapshot, exception_frame, exc_return);
    record('T');
    NVIC_ISPR0 = IRQ1_BIT;  /* UART-like IRQ becomes pending INSIDE Timer ISR */
    record('t');
}

__attribute__((used, noinline))
void uart_isr_body(uint32_t *exception_frame, uint32_t exc_return)
{
    save_snapshot(&uart_snapshot, exception_frame, exc_return);
    record('U');
    record('u');
}

__attribute__((naked)) void IRQ0_Handler(void)
{
    __asm volatile ("mrs r0, msp\n"
                    "mov r1, lr\n"
                    "b timer_isr_body\n");
}

__attribute__((naked)) void IRQ1_Handler(void)
{
    __asm volatile ("mrs r0, msp\n"
                    "mov r1, lr\n"
                    "b uart_isr_body\n");
}

static void semihost_print(const char *str)
{
    register uint32_t operation __asm("r0") = 0x04u; /* SYS_WRITE0 */
    register const char *argument __asm("r1") = str;
    __asm volatile ("bkpt #0xab" : "+r"(operation) : "r"(argument) : "memory");
}

static void print_hex(uint32_t value)
{
    char out[11] = "0x00000000";
    static const char digits[] = "0123456789ABCDEF";
    for (uint32_t i = 0; i < 8u; ++i) {
        out[2u + i] = digits[(value >> (28u - 4u * i)) & 0xFu];
    }
    semihost_print(out);
}

__attribute__((noreturn)) static void semihost_exit(uint32_t status)
{
    const uint32_t params[2] = { 0x20026u, status };
    register uint32_t operation __asm("r0") = 0x20u;
    register const uint32_t *argument __asm("r1") = params;
    __asm volatile ("bkpt #0xab" : "+r"(operation) : "r"(argument) : "memory");
    while (1) { }
}

static uint32_t matches(const char *expected)
{
    uint32_t i = 0u;
    while (expected[i] != '\0' && trace[i] == expected[i]) {
        ++i;
    }
    return (expected[i] == '\0' && trace[i] == '\0') ? 1u : 0u;
}

int main(void)
{
    record('M');

    /* Lower numerical priority = higher urgency. Both IRQs use the same
     * preemption priority grouping (default on reset). */
    NVIC_IPR0 = 0x40u;  /* Timer-like IRQ0: higher urgency */
    NVIC_IPR1 = 0x80u;  /* UART-like IRQ1: lower urgency */

#if DEFER_SECOND
    /* IRQ1 can become PENDING even though NVIC enable bit is clear. */
    NVIC_ISER0 = IRQ0_BIT;
#else
    NVIC_ISER0 = IRQ0_BIT | IRQ1_BIT;
#endif
    NVIC_ISPR0 = IRQ0_BIT;  /* Trigger Timer-like IRQ0. */

#if DEFER_SECOND
    record('m');             /* main resumed; UART-like IRQ still disabled */
    NVIC_ISER0 = IRQ1_BIT;   /* Enabling pending IRQ1 now allows its ISR */
    record('n');             /* main continues after IRQ1 finishes */
    const char *expected = "MTtmUun";
#else
    record('m');             /* first thread instruction runs AFTER UART ISR */
    const char *expected = "MTtUum";
#endif
    trace[trace_count] = '\0';

#if DEFER_SECOND
    semihost_print("Case: IRQ1 disabled until main resumes\n");
#else
    semihost_print("Case: IRQ1 already enabled and eligible\n");
#endif
    semihost_print("Trace: ");
    semihost_print((const char *)(const void *)trace);
    semihost_print("\nTimer frame MSP: "); print_hex(timer_snapshot.entry_msp);
    semihost_print("\nUART  frame MSP: "); print_hex(uart_snapshot.entry_msp);
    semihost_print("\nTimer stacked PC: "); print_hex(timer_snapshot.stacked_pc);
    semihost_print("\nUART  stacked PC: "); print_hex(uart_snapshot.stacked_pc);
    semihost_print("\nTimer EXC_RETURN: "); print_hex(timer_snapshot.exc_return);
    semihost_print("\nUART  EXC_RETURN: "); print_hex(uart_snapshot.exc_return);
    semihost_print("\nStacked PC identical: ");
    uint32_t same_pc = (timer_snapshot.stacked_pc == uart_snapshot.stacked_pc);
    semihost_print(same_pc ? "YES\n" : "NO\n");

    /* Sanity checks: both exceptions interrupted Thread mode on MSP.
     * Same saved PC in eligible case is CONSISTENT with retained frame;
     * a trace alone does NOT prove hardware cycle savings. */
    uint32_t ok = matches(expected);
    ok &= (timer_snapshot.exc_return == RETURN_TO_THREAD_MSP);
    ok &= (uart_snapshot.exc_return == RETURN_TO_THREAD_MSP);
    ok &= (timer_snapshot.ipsr == 16u && uart_snapshot.ipsr == 17u);
#if DEFER_SECOND
    ok &= !same_pc;
#else
    ok &= same_pc;
#endif
    semihost_print(ok ? "PASS: ordering and exception-state checks\n" :
                        "FAIL: check trace, EXC_RETURN and stacked PCs\n");
    semihost_exit(ok ? 0u : 1u);
}
