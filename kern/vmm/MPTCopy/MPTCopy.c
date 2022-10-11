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

/**
 * Copies page directory and page table entries of proc_from to the process proc_to
 * @param proc_from Process to copy page directory/table entries from
 * @param proc_to Process to copy page directory/table entries to
 * @return 0 if failure, 1 if success
 */
int copy_memory_map(unsigned int proc_from, unsigned int proc_to) {
    for (unsigned int pde_index = 0; pde_index < 1024; pde_index++) {
        if ((pde_index >= VM_USERLO_PDE) && (pde_index < VM_USERHI_PDE)) {
            // Only copy user-space memory page table entries
            unsigned int pdir_entry_from = get_pdir_entry(proc_from, pde_index);
            unsigned int pdir_entry_to = get_pdir_entry(proc_to, pde_index);
            if (pdir_entry_from == 0 && pdir_entry_to == 0) {
                continue;
            } else if (pdir_entry_from == 0) {
                // FROM page directory entry is empty, but TO page directory has allocated page table
                for (unsigned int pte_index = 0; pte_index < 1024; pte_index++) {
                    rmv_ptbl_entry(proc_to, pde_index, pte_index);
                }
            } else if (pdir_entry_to == 0) {
                // FROM page directory entry is mapped, but TO page directory entry is unmapped
                unsigned int page_table_page_index = alloc_ptbl(proc_to, pde_index << 22);
                if (page_table_page_index == 0) {
                    // Failure code - we cannot allocate a new page for a page table
                    return 0;
                }
                for (unsigned int pte_index = 0; pte_index < 1024; pte_index++) {
                    unsigned int ptbl_entry_from = get_ptbl_entry(proc_from, pde_index, pte_index);
                    unsigned int ptbl_entry_perms_original = ptbl_entry_from & 0x00000fff;
                    unsigned int ptbl_entry_page_index = ptbl_entry_from >> 12;
                    unsigned int ptbl_entry_perms_new = (ptbl_entry_perms_original | PTE_COW) & ~PTE_W;
                    set_ptbl_entry(proc_from, pde_index, pte_index, ptbl_entry_page_index, ptbl_entry_perms_new);
                    set_ptbl_entry(proc_to, pde_index, pte_index, ptbl_entry_page_index, ptbl_entry_perms_new);
                }
            } else {
                for (unsigned int pte_index = 0; pte_index < 1024; pte_index++) {
                    unsigned int ptbl_entry_from = get_ptbl_entry(proc_from, pde_index, pte_index);
                    unsigned int ptbl_entry_perms_original = ptbl_entry_from & 0x00000fff;
                    unsigned int ptbl_entry_page_index = ptbl_entry_from >> 12;
                    unsigned int ptbl_entry_perms_new = (ptbl_entry_perms_original | PTE_COW) & ~PTE_W;
                    set_ptbl_entry(proc_from, pde_index, pte_index, ptbl_entry_page_index, ptbl_entry_perms_new);
                    set_ptbl_entry(proc_to, pde_index, pte_index, ptbl_entry_page_index, ptbl_entry_perms_new);
                }
            }
        }
    }
    return 1;
}

/**
 * This function is invoked when a COW page at vaddr is being written to. This function copies that page to a new
 * physical page-frame and makes it writeable and non-COW.
 * @param pid Process ID
 * @param vaddr Virtual address of the COW read-only page that has a write-attempt
 * @return 0 if failure, 1 if success
 */
int copy_cow_page(unsigned int pid, unsigned int vaddr) {
    unsigned int cow_ptbl_entry = get_ptbl_entry_by_va(pid, vaddr);
    unsigned int cow_page_perms = cow_ptbl_entry & 0x00000fff;
    unsigned int cow_page_frame = cow_ptbl_entry >> 12;

    // Make copied page writeable and non-COW
    unsigned int new_perms = (cow_page_perms | PTE_W) & ~PTE_COW;
    if (alloc_page(pid, vaddr, new_perms) == MagicNumber) {
        // Return failure code if page allocation fails
        return 0;
    }

    // Copy the contents of the COW page to the target page
    unsigned int target_page_frame = get_ptbl_entry_by_va(pid, vaddr) >> 12;
    for (unsigned int byte_index = 0; byte_index < PAGESIZE; byte_index++) {
        char * cow_paddr = (void *) ((cow_page_frame << 12) | byte_index);
        char * target_paddr = (void *) ((target_page_frame << 12) | byte_index);
        *target_paddr = *cow_paddr;
    }
    return 1;
}
