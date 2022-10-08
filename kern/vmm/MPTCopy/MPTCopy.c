#include <lib/gcc.h>
#include <lib/x86.h>
#include <lib/debug.h>

#include "import.h"

#define PAGESIZE      4096
#define PDIRSIZE      (PAGESIZE * 1024)
#define VM_USERLO     0x40000000
#define VM_USERHI     0xF0000000
#define VM_USERLO_PDE (VM_USERLO / PDIRSIZE)
#define VM_USERHI_PDE (VM_USERHI / PDIRSIZE)

void copy_memory_map(unsigned int proc_from, unsigned int proc_to) {
    for (unsigned int pde_index = 0; pde_index < 1024; pde_index++) {
        if ((pde_index >= VM_USERLO_PDE) || (VM_USERHI_PDE > pde_index)) {
            // User-space memory
            unsigned int pdir_entry = get_pdir_entry(proc_from, pde_index);

            set_pdir_entry(proc_to, )
        }
    }
    return;
}