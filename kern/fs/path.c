// File system implementation.  Five layers:
//   + Blocks: allocator for raw disk blocks.
//   + Log: crash recovery for multi-step updates.
//   + Files: inode allocator, reading, writing, metadata.
//   + Directories: inode with special contents (list of other inodes!)
//   + Names: paths like /usr/fs.c for convenient naming.
//
// This file contains the low-level file system manipulation
// routines.  The (higher-level) system call implementations
// are in sysfile.c.

#include <kern/lib/types.h>
#include <kern/lib/debug.h>
#include <kern/lib/spinlock.h>
#include <thread/PTCBIntro/export.h>
#include <thread/PCurID/export.h>
#include "inode.h"
#include "dir.h"
#include "log.h"

// Paths

/**
 * Copy the next path element from path into name.
 * If the length of name is larger than or equal to DIRSIZ, then only
 * (DIRSIZ - 1) # characters should be copied into name.
 * This is because you need to save '\0' in the end.
 * You should still skip the entire string in this case.
 * Return a pointer to the element following the copied one.
 * The returned path has no leading slashes,
 * so the caller can check *path == '\0' to see if the name is the last one.
 * If no name to remove, return 0.
 *
 * Examples :
 *   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
 *   skipelem("///a//bb", name) = "bb", setting name = "a"
 *   skipelem("a", name) = "", setting name = "a"
 *   skipelem("", name) = skipelem("////", name) = 0
 */
static char *skipelem(volatile char *path, volatile char *name)
{
    unsigned int i = 0;
    while (path[i] == '/')
    {
        i++;
    }
    unsigned int start = i;
    while (path[i] != '/' && path[i] != '\0')
    {
        i++;
    }
    // KERN_INFO("i: %d, start: %d\n",i, start);
    if (i - start == 0)
    {
        return 0;
    }

    if (i - start >= DIRSIZ)
    {
        // Exceeds bounds
        strncpy(name, &path[start], DIRSIZ - 1);
        name[DIRSIZ - 1] = '\0';
    }
    else
    {
        // Within bounds
        strncpy(name, &path[start], i - start);
        name[i - start] = '\0';
    }

    while (path[i] == '/')
    {
        i++;
    }
    // KERN_INFO("path[i]: %d\n", path[i]);
    return &path[i];
}

/**
 * Look up and return the inode for a path name.
 * If nameiparent is true, return the inode for the parent and copy the final
 * path element into name, which must have room for DIRSIZ bytes.
 * Returns 0 in the case of error.
 */
static struct inode *namex(char *path, bool nameiparent, char *name)
{
    struct inode *ip;
    // KERN_INFO("namex path=%s, nameiparent=%d\n", path, nameiparent);
    // If path is a full path, get the pointer to the root inode. Otherwise get
    // the inode corresponding to the current working directory.
    if (*path == '/')
    {
        ip = inode_get(ROOTDEV, ROOTINO);
    }
    else
    {
        ip = inode_dup((struct inode *)tcb_get_cwd(get_curid()));
    }

    while ((path = skipelem(path, name)) != 0)
    {
        // KERN_INFO("Hi path=%s name=%s\n", path, name);
        inode_lock(ip);
        // KERN_INFO("namex iter path=%s name=%s\n", path, name);
        uint32_t poff;
        if (ip->type != T_DIR)
        {
            inode_unlockput(ip);
            return 0;
        }
        // KERN_INFO("info: path[0]: %c", path[0]);
        if (nameiparent && path[0] == '\0')
        {
            // KERN_INFO("return parent ip inum=%d type=%d name=%s\n", ip->inum, ip->type, name);
            inode_unlock(ip);
            return ip;
        }
        struct inode *child = dir_lookup(ip, name, &poff);
        inode_unlockput(ip);

        // KERN_INFO("child inode pointer=%p\n", ip);
        // if (child != 0) {
        //     KERN_INFO("inode inum=%d type=%d\n", ip);\
        // }
        ip = child;

        if (child == 0)
        {
            return 0;
        }
    }
    // inode_dup(ip);
    // KERN_INFO("namex: returning inum=%d\n", ip->inum);
    return ip;
}

/**
 * Return the inode corresponding to path.
 */
struct inode *namei(char *path)
{
    char name[DIRSIZ];
    return namex(path, FALSE, name);
}

/**
 * Return the inode corresponding to path's parent directory and copy the final
 * element into name.
 */
struct inode *nameiparent(char *path, char *name)
{
    return namex(path, TRUE, name);
}
