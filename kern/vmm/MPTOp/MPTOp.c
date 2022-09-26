#include <lib/x86.h>

#include "import.h"

unsigned int get_pde_index(unsigned int vaddr)
{
    return vaddr >> 22;
}

unsigned int get_pte_index(unsigned int vaddr)
{
    return (vaddr << 10) >> 22;
}

unsigned int get_page_offset(unsigned int vaddr)
{
    return (vaddr << 20) >> 20;
}
/**
 * Returns the page table entry corresponding to the virtual address,
 * according to the page structure of process # [proc_index].
 * Returns 0 if the mapping does not exist.
 */
unsigned int get_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    // TODO Make note about returning 0, since if mapping doesn't exist entry is 0
    unsigned int pde_index = get_pde_index(vaddr);
    unsigned int pte_index = get_pte_index(vaddr);

    if (get_pdir_entry(proc_index, pde_index) == 0) {
        return 0;
    }
    unsigned int entry = get_ptbl_entry(proc_index, pde_index, pte_index);
    return entry;
}

// Returns the page directory entry corresponding to the given virtual address.
unsigned int get_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int pde_index = get_pde_index(vaddr);

    return get_pdir_entry(proc_index, pde_index);
}

// Removes the page table entry for the given virtual address.
void rmv_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int pde_index = get_pde_index(vaddr);
    unsigned int pte_index = get_pte_index(vaddr);
    rmv_ptbl_entry(proc_index, pde_index, pte_index);
}

// Removes the page directory entry for the given virtual address.
void rmv_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int pde_index = get_pde_index(vaddr);
    rmv_pdir_entry(proc_index, pde_index);
}

// Maps the virtual address [vaddr] to the physical page # [page_index] with permission [perm].
// You do not need to worry about the page directory entry. just map the page table entry.
void set_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr,
                          unsigned int page_index, unsigned int perm)
{

    unsigned int pde_index = get_pde_index(vaddr);
    unsigned int pte_index = get_pte_index(vaddr);
    set_ptbl_entry(proc_index, pde_index, pte_index, page_index, perm);
}

// Registers the mapping from [vaddr] to physical page # [page_index] in the page directory.
void set_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr,
                          unsigned int page_index)
{
    unsigned int pde_index = get_pde_index(vaddr);
    set_pdir_entry(proc_index, pde_index, page_index);
}

// Initializes the identity page table.
// The permission for the kernel memory should be PTE_P, PTE_W, and PTE_G,
// While the permission for the rest should be PTE_P and PTE_W.
void idptbl_init(unsigned int mbi_addr)
{
    // TODO: Ask about how to differentiate where Kernel memory is vs rest of memory
    // - Is there a macro VM_USERLO? 
    container_init(mbi_addr);
    for(unsigned int pde_index = 0; pde_index < 1024; pde_index++){
        for(unsigned int pte_index = 0; pte_index < 1024; pte_index++){
            set_ptbl_entry_identity(pde_index, pte_index, PTE_P | PTE_W | PTE_G);
        }
    }
    // TODO
}
