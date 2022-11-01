#include <lib/trap.h>
#include <lib/debug.h>
#include <lib/string.h>
#include <dev/intr.h>
#include "import.h"

#define KERN_INFO_CPU(str, idx) \
    if (idx == 0) KERN_INFO("[BSP KERN] " str); \
    else KERN_INFO("[AP%d KERN] " str, idx);

int inited = FALSE;

trap_cb_t TRAP_HANDLER[NUM_CPUS][256];

void trap_init_array(void)
{
    KERN_ASSERT(inited == FALSE);
    memzero(&TRAP_HANDLER, sizeof(trap_cb_t) * 8 * 256);
    inited = TRUE;
}

void trap_handler_register(unsigned int cpu_idx, int trapno, trap_cb_t cb)
{
    KERN_ASSERT(0 <= cpu_idx && cpu_idx < 8);
    KERN_ASSERT(0 <= trapno && trapno < 256);
    KERN_ASSERT(cb != NULL);

    TRAP_HANDLER[cpu_idx][trapno] = cb;
}

void trap_init(unsigned int cpu_idx)
{
    if (cpu_idx == 0) {
        trap_init_array();
    }

    KERN_INFO_CPU("Register trap handlers...\n", cpu_idx);

    trap_handler_register(cpu_idx, T_DIVIDE, exception_handler);
    trap_handler_register(cpu_idx, T_DEBUG, exception_handler);
    trap_handler_register(cpu_idx, T_NMI, exception_handler);
    trap_handler_register(cpu_idx, T_BRKPT, exception_handler);
    trap_handler_register(cpu_idx, T_OFLOW, exception_handler);
    trap_handler_register(cpu_idx, T_BOUND, exception_handler);
    trap_handler_register(cpu_idx, T_ILLOP, exception_handler);
    trap_handler_register(cpu_idx, T_DEVICE, exception_handler);
    trap_handler_register(cpu_idx, T_DBLFLT, exception_handler);
    trap_handler_register(cpu_idx, T_COPROC, exception_handler);
    trap_handler_register(cpu_idx, T_TSS, exception_handler);
    trap_handler_register(cpu_idx, T_SEGNP, exception_handler);
    trap_handler_register(cpu_idx, T_STACK, exception_handler);
    trap_handler_register(cpu_idx, T_GPFLT, exception_handler);
    trap_handler_register(cpu_idx, T_PGFLT, exception_handler);
    trap_handler_register(cpu_idx, T_RES, exception_handler);
    trap_handler_register(cpu_idx, T_FPERR, exception_handler);
    trap_handler_register(cpu_idx, T_ALIGN, exception_handler);
    trap_handler_register(cpu_idx, T_MCHK, exception_handler);
    trap_handler_register(cpu_idx, T_SIMD, exception_handler);
    trap_handler_register(cpu_idx, T_SECEV, exception_handler);

    trap_handler_register(cpu_idx, T_IRQ0 + IRQ_TIMER, interrupt_handler);
    trap_handler_register(cpu_idx, T_IRQ0 + IRQ_KBD, interrupt_handler);
    trap_handler_register(cpu_idx, T_IRQ0 + IRQ_SLAVE, interrupt_handler);
    trap_handler_register(cpu_idx, T_IRQ0 + IRQ_SERIAL24, interrupt_handler);
    trap_handler_register(cpu_idx, T_IRQ0 + IRQ_SERIAL13, interrupt_handler);
    trap_handler_register(cpu_idx, T_IRQ0 + IRQ_LPT2, interrupt_handler);
    trap_handler_register(cpu_idx, T_IRQ0 + IRQ_FLOPPY, interrupt_handler);
    trap_handler_register(cpu_idx, T_IRQ0 + IRQ_SPURIOUS, interrupt_handler);
    trap_handler_register(cpu_idx, T_IRQ0 + IRQ_RTC, interrupt_handler);
    trap_handler_register(cpu_idx, T_IRQ0 + IRQ_MOUSE, interrupt_handler);
    trap_handler_register(cpu_idx, T_IRQ0 + IRQ_COPROCESSOR, interrupt_handler);
    trap_handler_register(cpu_idx, T_IRQ0 + IRQ_IDE1, interrupt_handler);
    trap_handler_register(cpu_idx, T_IRQ0 + IRQ_IDE2, interrupt_handler);

    trap_handler_register(cpu_idx, T_SYSCALL, syscall_dispatch);

    trap_handler_register(cpu_idx, T_LTIMER, interrupt_handler);
    trap_handler_register(cpu_idx, T_LERROR, interrupt_handler);
    trap_handler_register(cpu_idx, T_PERFCTR, interrupt_handler);
    trap_handler_register(cpu_idx, T_IPI0 + IPI_RESCHED, interrupt_handler);
    trap_handler_register(cpu_idx, T_IPI0 + IPI_INVALC, interrupt_handler);
    trap_handler_register(cpu_idx, T_DEFAULT, interrupt_handler);

//    for (int trapno = 0; trapno < 256; trapno++) {
//        if (trapno == T_SYSCALL) {
//            trap_handler_register(cpu_idx, T_SYSCALL, syscall_dispatch);
//        } else if (trapno >= 0 && trapno <= 31) {
//            trap_handler_register(cpu_idx, trapno, exception_handler);
//        } else {
//            trap_handler_register(cpu_idx, trapno, interrupt_handler);
//        }
//    }

    KERN_INFO_CPU("Done.\n", cpu_idx);
    KERN_INFO_CPU("Enabling interrupts...\n", cpu_idx);

    /* enable interrupts */
    intr_enable(IRQ_TIMER, cpu_idx);
    intr_enable(IRQ_KBD, cpu_idx);
    intr_enable(IRQ_SERIAL13, cpu_idx);

    KERN_INFO_CPU("Done.\n", cpu_idx);
}
