#include <lib/elf.h>
#include <lib/debug.h>
#include <lib/gcc.h>
#include <lib/seg.h>
#include <lib/trap.h>
#include <lib/x86.h>

#include "import.h"

extern tf_t uctx_pool[NUM_IDS];
extern char STACK_LOC[NUM_IDS][PAGESIZE];

void proc_start_user(void)
{
    unsigned int cur_pid = get_curid();
    tss_switch(cur_pid);
    set_pdir_base(cur_pid);

    trap_return((void *) &uctx_pool[cur_pid]);
}

unsigned int proc_create(void *elf_addr, unsigned int quota)
{
    unsigned int pid, id;

    id = get_curid();
    pid = thread_spawn((void *) proc_start_user, id, quota);

    if (pid != NUM_IDS) {
        elf_load(elf_addr, pid);

        uctx_pool[pid].es = CPU_GDT_UDATA | 3;
        uctx_pool[pid].ds = CPU_GDT_UDATA | 3;
        uctx_pool[pid].cs = CPU_GDT_UCODE | 3;
        uctx_pool[pid].ss = CPU_GDT_UDATA | 3;
        uctx_pool[pid].esp = VM_USERHI;
        uctx_pool[pid].eflags = FL_IF;
        uctx_pool[pid].eip = elf_entry(elf_addr);
    }

    return pid;
}

unsigned int proc_fork() {
    unsigned int current = get_curid();
    unsigned int remaining_quota = container_get_quota(current) - container_get_usage(current);
    unsigned int fork_quota = remaining_quota / 2;
    unsigned int fork_pid = thread_spawn((void *) proc_start_user, current, fork_quota);
    if (fork_pid != NUM_IDS) {
        uctx_pool[fork_pid] = uctx_pool[current];
        uctx_pool[current].regs.ebx = fork_pid;
        uctx_pool[fork_pid].regs.ebx = 0;
        copy_memory_map(current, fork_pid);
    }
    return fork_pid;
}
