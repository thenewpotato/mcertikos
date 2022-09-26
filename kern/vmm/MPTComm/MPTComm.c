#include <lib/x86.h>

#include "import.h"

#define PAGESIZE     4096
#define VM_USERLO    0x40000000
#define VM_USERHI    0xF0000000
#define VM_USERLO_PI (VM_USERLO / PAGESIZE)
#define VM_USERHI_PI (VM_USERHI / PAGESIZE)

#define PDE_OF_ADDR(addr)   (addr >> 22)
#define VM_USERLO_PDE       PDE_OF_ADDR(VM_USERLO)
#define VM_USERHI_PDE       PDE_OF_ADDR(VM_USERHI)

/**
 * For each process from id 0 to NUM_IDS - 1,
 * set up the page directory entries so that the kernel portion of the map is
 * the identity map, and the rest of the page directories are unmapped.
 */
void pdir_init(unsigned int mbi_addr)
{
    idptbl_init(mbi_addr);

    for (unsigned int proc = 0; proc < NUM_IDS; proc++) {
        for (unsigned int pde = 0; pde < 1024; pde++) {
            if (pde < VM_USERLO_PDE || pde >= VM_USERHI_PDE) {
                set_pdir_entry_identity(proc, pde);
            } else {
                // TODO: Are user entries already 0? Or do we set them to 0?
            }
        }
    }
}

/**
 * Allocates a page (with container_alloc) for the page table,
 * and registers it in the page directory for the given virtual address,
 * and clears (set to 0) all page table entries for this newly mapped page table.
 * It returns the page index of the newly allocated physical page.
 * In the case when there's no physical page available, it returns 0.
 */
unsigned int alloc_ptbl(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int page_index = container_alloc(proc_index);
    if (page_index == 0) {
        // No physical page available
        return 0;
    }

    // Set all page table entries to 0
    unsigned int page_addr = page_index * PAGESIZE;
    unsigned int *page_table = (unsigned int *) page_addr;
    for (unsigned int i = 0; i < 1024; i++) {
        page_table[i] = 0;
    }

    // Register page table in page directory
    set_pdir_entry_by_va(proc_index, vaddr, page_index);

    return page_index;
}

// Reverse operation of alloc_ptbl.
// Removes corresponding the page directory entry,
// and frees the page for the page table entries (with container_free).
void free_ptbl(unsigned int proc_index, unsigned int vaddr)
{
    // pdir_entry stores the address of the page table (ORed with permission bits)
    unsigned int pdir_entry = get_pdir_entry_by_va(proc_index, vaddr);

    // Retrieve the page index of where the page table is physically located
    unsigned int page_index = pdir_entry >> 12;

    // Disassociate page table from page directory
    rmv_pdir_entry_by_va(proc_index, vaddr);

    // Frees the physical page that used to store the page table
    container_free(proc_index, page_index);
}
