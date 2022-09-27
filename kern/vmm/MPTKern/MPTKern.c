#include <lib/x86.h>
#include <lib/debug.h>

#include "import.h"

#define VM_USERLO    0x40000000
#define VM_USERHI    0xF0000000
#define VM_USERLO_PI (VM_USERLO / PAGESIZE)
#define VM_USERHI_PI (VM_USERHI / PAGESIZE)

#define PDE_OF_ADDR(addr)   (addr >> 22)
#define PTE_OF_ADDR(addr)   ((addr << 10) >> 22)

/**
 * Sets the entire page map for process 0 as the identity map.
 * Note that part of the task is already completed by pdir_init.
 */
void pdir_init_kern(unsigned int mbi_addr)
{
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
    // If page directory entry does not exist, allocate a page for the page table and register it in the page directory
    if (get_pdir_entry_by_va(proc_index, vaddr) == 0) {
        // If page allocation fails, return error code
        if (alloc_ptbl(proc_index, vaddr) == 0) {
            return MagicNumber;
        }
    }

    // Fill in page table entry
    set_ptbl_entry_by_va(proc_index, vaddr, page_index, perm);

    // Returns physical page index registered in the page directory
    unsigned int pdir_entry = get_pdir_entry_by_va(proc_index, vaddr);
    unsigned int ptbl_page_index = pdir_entry >> 12;
    return ptbl_page_index;
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
    unsigned int ptbl_entry = get_ptbl_entry_by_va(proc_index, vaddr);

    // If page table entry does not exist, immediately return
    if (ptbl_entry == 0) {
        return 0;
    }

    rmv_ptbl_entry_by_va(proc_index, vaddr);
    return ptbl_entry;
}
