#include <lib/x86.h>

#include "import.h"

/**
 * This function will be called when there's no mapping found in the page structure
 * for the given virtual address [vaddr], e.g., by the page fault handler when
 * a page fault happened because the user process accessed a virtual address
 * that is not mapped yet.
 * The task of this function is to allocate a physical page and use it to register
 * a mapping for the virtual address with the given permission.
 * It should return the physical page index registered in the page directory, the
 * return value from map_page.
 * In the case of error, it should return the constant MagicNumber.
 */
unsigned int alloc_page(unsigned int proc_index, unsigned int vaddr,
                        unsigned int perm)
{
    unsigned int new_page = container_alloc(proc_index);
    if (new_page == 0) {
        // Failed to allocate page (e.g., quota exceeded), return error code
        return MagicNumber;
    }
    
    unsigned int pdir_entry_index = map_page(proc_index, vaddr, new_page, perm);
    if (pdir_entry_index == MagicNumber) {
        // Failed to map page, clean up and return error code
        container_free(proc_index, new_page);
        return MagicNumber;
    }

    return pdir_entry_index;
}

/**
 * Designate some memory quota for the next child process.
 */
unsigned int alloc_mem_quota(unsigned int id, unsigned int quota)
{
    unsigned int child;
    child = container_split(id, quota);
    return child;
}
