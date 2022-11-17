#include <kern/lib/types.h>
#include <kern/lib/debug.h>
#include <kern/lib/string.h>
#include "inode.h"
#include "dir.h"

// Directories
int dir_namecmp(const char *s, const char *t)
{
    return strncmp(s, t, DIRSIZ);
}

/**
 * Look for a directory entry in a directory.
 * If found, set *poff to byte offset of entry.
 */
struct inode *dir_lookup(struct inode *dp, char *name, uint32_t *poff)
{
    uint32_t off, inum;
    struct dirent de;

    if (dp->type != T_DIR)
        KERN_PANIC("dir_lookup not DIR");

    for (unsigned int offset = 0; offset < dp->size; offset += sizeof(struct dirent))
    {
        struct dirent sub_dir;

        inode_read(dp, (char *)(&sub_dir), offset, sizeof(struct dirent));

        if (sub_dir.inum != 0 && dir_namecmp(name, sub_dir.name) == 0)
        {
            if (poff != NULL) {
                *poff = offset;
            }
            return inode_get(dp->dev, sub_dir.inum);
        }
    }

    return 0;
}

// Write a new directory entry (name, inum) into the directory dp.
int dir_link(struct inode *dp, char *name, uint32_t inum)
{
    uint32_t poff;
    struct inode *sub_directory = dir_lookup(dp, name, &poff);
    // sub_directory already in directory
    if (sub_directory != 0)
    {
        inode_put(sub_directory);
        return -1;
    }

    for (unsigned int offset = 0; offset < dp->size; offset += sizeof(struct dirent))
    {
        struct dirent sub_dir;
        inode_read(dp, (char *)(&sub_dir), offset, sizeof(struct dirent));

        if (sub_dir.inum == 0)
        {
            sub_dir.inum = inum;
            strncpy(sub_dir.name, name, DIRSIZ);

            int num_bytes = inode_write(dp, (char *)(&sub_dir), offset, sizeof(struct dirent));

            if (num_bytes != sizeof(struct dirent))
            {
                KERN_PANIC("dir_link bad write");
            }
            return 0;
        }
    }

    KERN_PANIC("dir_link directory full");

    return -1;
}
