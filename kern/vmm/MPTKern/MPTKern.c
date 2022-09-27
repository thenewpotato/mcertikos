#include <lib/x86.h>
#include <lib/debug.h>

#include "import.h"

#define PAGESIZE 4096
#define VM_USERLO 0x40000000
#define VM_USERHI 0xF0000000
#define VM_USERLO_PI (VM_USERLO / PAGESIZE)
#define VM_USERHI_PI (VM_USERHI / PAGESIZE)

#define PDE_OF_ADDR(addr) (addr >> 22)
#define VM_USERLO_PDE PDE_OF_ADDR(VM_USERLO)
#define VM_USERHI_PDE PDE_OF_ADDR(VM_USERHI)
/**
 * Sets the entire page map for process 0 as the identity map.
 * Note that part of the task is already completed by pdir_init.
 */
void pdir_init_kern(unsigned int mbi_addr)
{
    // TODO: Define your local variables here.

    pdir_init(mbi_addr);
    unsigned int proc = 0;
    for (unsigned int pde = 0; pde < 1024; pde++)
    {
        set_pdir_entry_identity(proc, pde);
    }
}

/**
 * Maps the physical page # [page_index] for the given virtual address with the given permission.
 * In the case when the page table for the page directory entry is not set up,
 * you need to allocate the page table first.
 * In the case of error, it returns the constant MagicNumber defined in lib/x86.h,
 * otherwise, it returns the physical page index registered in the page directory,
 * (the return value of get_pdir_entry_by_va or alloc_ptbl).
 */
unsigned int map_page(unsigned int proc_index, unsigned int vaddr,
                      unsigned int page_index, unsigned int perm)
{
    // TODO
    // unsigned int physical_page_index = alloc_ptbl(proc_index, vaddr);
    // if (physical_page_index == 0)
    // {
    //     return MagicNumber;
    // }

    // If the current page directory entry is unallocated then we need to allocate a new pagetable

    return 0;
}

/**
 * Remove the mapping for the given virtual address (with rmv_ptbl_entry_by_va).
 * You need to first make sure that the mapping is still valid,
 * e.g., by reading the page table entry for the virtual address.
 * Nothing should be done if the mapping no longer exists.
 * You do not need to unmap the page table from the page directory.
 * It should return the corresponding page table entry.
 */
unsigned int unmap_page(unsigned int proc_index, unsigned int vaddr)
{
    // TODO
    return 0;
}
