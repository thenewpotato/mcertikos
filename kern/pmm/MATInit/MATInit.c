#include <lib/debug.h>
#include "import.h"

#define PAGESIZE     4096
#define VM_USERLO    0x40000000
#define VM_USERHI    0xF0000000
#define VM_USERLO_PI (VM_USERLO / PAGESIZE)
#define VM_USERHI_PI (VM_USERHI / PAGESIZE)

// Computes ceil(x, y).
unsigned int ceil(unsigned int x, unsigned int y) {
    if (x % y == 0) {
        return x / y;
    } else {
        return (x / y) + 1;
    }
}

/**
 * The initialization function for the allocation table AT.
 * It contains two major parts:
 * 1. Calculate the actual physical memory of the machine, and sets the number
 *    of physical pages (NUM_PAGES).
 * 2. Initializes the physical allocation table (AT) implemented in the MATIntro layer
 *    based on the information available in the physical memory map table.
 *    Review import.h in the current directory for the list of available
 *    getter and setter functions.
 */
void pmem_init(unsigned int mbi_addr)
{
    unsigned int nps;

    unsigned int highest_addr = 0;
    unsigned int nrows;

    // Calls the lower layer initialization primitive.
    // The parameter mbi_addr should not be used in the further code.
    devinit(mbi_addr);

    /**
     * Calculate the total number of physical pages provided by the hardware and
     * store it into the local variable nps.
     * Hint: Think of it as the highest address in the ranges of the memory map table,
     *       divided by the page size.
     */
    nrows = get_size();
    // Loop through rows in the physical memory map table and find the highest address in the ranges of the table
    for (unsigned int i = 0; i < nrows; i++) {
        unsigned int end_addr = get_mms(i) + get_mml(i);
        if (end_addr > highest_addr) {
            highest_addr = end_addr;
        }
    }
    // Rounding down is okay, because we cannot make use of partial pages
    nps = highest_addr / PAGESIZE;

    set_nps(nps);  // Setting the value computed above to NUM_PAGES.

    /**
     * Initialization of the physical allocation table (AT).
     *
     * In CertiKOS, all addresses < VM_USERLO or >= VM_USERHI are reserved by the kernel.
     * That corresponds to the physical pages from 0 to VM_USERLO_PI - 1,
     * and from VM_USERHI_PI to NUM_PAGES - 1.
     * The rest of the pages that correspond to addresses [VM_USERLO, VM_USERHI)
     * can be used freely ONLY IF the entire page falls into one of the ranges in
     * the memory map table with the permission marked as usable.
     *
     * Hint:
     * 1. You have to initialize AT for all the page indices from 0 to NPS - 1.
     * 2. For the pages that are reserved by the kernel, simply set its permission to 1.
     *    Recall that the setter at_set_perm also marks the page as unallocated.
     *    Thus, you don't have to call another function to set the allocation flag.
     * 3. For the rest of the pages, explore the memory map table to set its permission
     *    accordingly. The permission should be set to 2 only if there is a range
     *    containing the entire page that is marked as available in the memory map table.
     *    Otherwise, it should be set to 0. Note that the ranges in the memory map are
     *    not aligned by pages, so it may be possible that for some pages, only some of
     *    the addresses are in a usable range. Currently, we do not utilize partial pages,
     *    so in that case, you should consider those pages as unavailable.
     */
    // Initialize all non-kernel pages to unavailable
    for (unsigned int i = VM_USERLO_PI; i < VM_USERHI_PI; i++) {
        at_set_perm(i, 0);
    }
    // Loop through memory table rows to find available pages
    for (unsigned int rowindex = 0; rowindex < nrows; rowindex++) {
        // Range boundaries are [range_start_addr, range_end_addr)
        unsigned int range_start_addr = get_mms(rowindex);
        unsigned int range_end_addr = range_start_addr + get_mml(rowindex);
        unsigned int usable = is_usable(rowindex);
        // Page boundaries are [pageindex*PAGESIZE, (pageindex+1)*PAGESIZE)
        for (unsigned int pageindex = ceil(range_start_addr, PAGESIZE); pageindex < range_end_addr / PAGESIZE; pageindex++) {
            if (usable) {
                at_set_perm(pageindex, 2);
            } else {
                at_set_perm(pageindex, 0);
            }
        }
    }
    // Set kernel page permissions
    for (unsigned int i = 0; i < VM_USERLO_PI; i++) {
        at_set_perm(i, 1);
    }
    for (unsigned int i = VM_USERHI_PI; i < nps; i++) {
        at_set_perm(i, 1);
    }
}
