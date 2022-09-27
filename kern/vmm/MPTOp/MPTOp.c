#include <lib/x86.h>

#include "import.h"

#define PAGESIZE     4096
#define VM_USERLO    0x40000000
#define VM_USERHI    0xF0000000
#define VM_USERLO_PI (VM_USERLO / PAGESIZE)
#define VM_USERHI_PI (VM_USERHI / PAGESIZE)

#define PDE_OF_ADDR(addr)   (addr >> 22)
#define PTE_OF_ADDR(addr)   ((addr << 10) >> 22)
#define PI_FROM_PDE_PTE(pde,pte)  ((pde << 10) | pte)

/**
 * Returns the page table entry corresponding to the virtual address,
 * according to the page structure of process # [proc_index].
 * Returns 0 if the mapping does not exist.
 */
unsigned int get_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int pde_index = PDE_OF_ADDR(vaddr);
    unsigned int pte_index = PTE_OF_ADDR(vaddr);

    // "Not exist" means (Ed #107)
    // - The page directory entry corresponding to that virtual address is 0; or
    // - The page table entry corresponding to that virtual address is 0.
    if (get_pdir_entry(proc_index, pde_index) == 0) {
        return 0;
    }
    unsigned int entry = get_ptbl_entry(proc_index, pde_index, pte_index);
    return entry;
}

// Returns the page directory entry corresponding to the given virtual address.
unsigned int get_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int pde_index = PDE_OF_ADDR(vaddr);

    return get_pdir_entry(proc_index, pde_index);
}

// Removes the page table entry for the given virtual address.
void rmv_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int pde_index = PDE_OF_ADDR(vaddr);
    unsigned int pte_index = PTE_OF_ADDR(vaddr);
    rmv_ptbl_entry(proc_index, pde_index, pte_index);
}

// Removes the page directory entry for the given virtual address.
void rmv_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int pde_index = PDE_OF_ADDR(vaddr);
    rmv_pdir_entry(proc_index, pde_index);
}

// Maps the virtual address [vaddr] to the physical page # [page_index] with permission [perm].
// You do not need to worry about the page directory entry. just map the page table entry.
void set_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr,
                          unsigned int page_index, unsigned int perm)
{

    unsigned int pde_index = PDE_OF_ADDR(vaddr);
    unsigned int pte_index = PTE_OF_ADDR(vaddr);
    set_ptbl_entry(proc_index, pde_index, pte_index, page_index, perm);
}

// Registers the mapping from [vaddr] to physical page # [page_index] in the page directory.
void set_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr,
                          unsigned int page_index)
{
    unsigned int pde_index = PDE_OF_ADDR(vaddr);
    set_pdir_entry(proc_index, pde_index, page_index);
}

// Initializes the identity page table.
// The permission for the kernel memory should be PTE_P, PTE_W, and PTE_G,
// While the permission for the rest should be PTE_P and PTE_W.
void idptbl_init(unsigned int mbi_addr)
{
    container_init(mbi_addr);

    for(unsigned int pde_index = 0; pde_index < 1024; pde_index++){
        for(unsigned int pte_index = 0; pte_index < 1024; pte_index++){
            unsigned int page_index = PI_FROM_PDE_PTE(pde_index, pte_index);
            if (page_index >= VM_USERLO_PI && page_index < VM_USERHI_PI) {
                // This page is in user-space
                set_ptbl_entry_identity(pde_index, pte_index, PTE_P | PTE_W);
            } else {
                // This page is in kernel-space
                set_ptbl_entry_identity(pde_index, pte_index, PTE_P | PTE_W | PTE_G);
            }
        }
    }
}
