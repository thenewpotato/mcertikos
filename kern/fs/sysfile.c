// File-system system calls.

#include <kern/lib/types.h>
#include <kern/lib/debug.h>
#include <kern/lib/pmap.h>
#include <kern/lib/string.h>
#include <kern/lib/trap.h>
#include <kern/lib/syscall.h>
#include <kern/thread/PTCBIntro/export.h>
#include <kern/thread/PCurID/export.h>
#include <kern/trap/TSyscallArg/export.h>

#include "dir.h"
#include "path.h"
#include "file.h"
#include "fcntl.h"
#include "log.h"

#define KERNEL_BUFFER_SIZE 10000

char kernel_buffer[KERNEL_BUFFER_SIZE];

// file system init -> dev init

/**
 * This function is not a system call handler, but an auxiliary function
 * used by sys_open.
 * Allocate a file descriptor for the given file.
 * You should scan the list of open files for the current thread
 * and find the first file descriptor that is available.
 * Return the found descriptor or -1 if none of them is free.
 */
static int fdalloc(struct file *f)
{
    struct file **open_files = tcb_get_openfiles(get_curid());
    for (unsigned int i = 0; i < NOFILE; i++)
    {
        if (open_files[i] == NULL)
        {
            tcb_set_openfiles(get_curid(), i, f);
            return i;
        }
    }

    return -1;
}

/**
 * From the file indexed by the given file descriptor, read n bytes and save them
 * into the buffer in the user. As explained in the assignment specification,
 * you should first write to a kernel buffer then copy the data into user buffer
 * with pt_copyout.
 * Return Value: Upon successful completion, read() shall return a non-negative
 * integer indicating the number of bytes actually read. Otherwise, the
 * functions shall return -1 and set errno E_BADF to indicate the error.
 */

void sys_read(tf_t *tf)
{
    // int fd = syscall_get_arg2(tf);
    // unsigned int buf = syscall_get_arg3(tf);
    // size_t nbytes = syscall_get_arg4(tf);

    // unsigned int cur_pid = get_curid();

    // struct file **open_files = tcb_get_openfiles(cur_pid);

    // struct file *indexed_file = open_files[fd];

    // // if not in file descriptpor table or the number of bytes exceeds
    // if (nbytes >= 10000 || indexed_file == NULL)
    // {
    //     syscall_set_errno(tf, -1);
    //     return;
    // }
    // else
    // {
    //     if (file_read(indexed_file, kernel_buffer, nbytes) == -1)
    //     {
    //         syscall_set_errno(tf, -1);
    //         return;
    //     }
    //     if (pt_copyout(kernel_buffer, cur_pid, p, nbytes) == -1)
    //     {
    //         syscall_set_errno(tf, -1);
    //         return;
    //     }
    // }
    return;
}
/**
 * Write n bytes of data in the user's buffer to the file indexed by the file descriptor.
 * You should first copy the data info an in-kernel buffer with pt_copyin and then
 * pass this buffer to appropriate file manipulation function.
 * Upon successful completion, write() shall return the number of bytes actually
 * written to the file associated with f. This number shall never be greater
 * than nbyte. Otherwise, -1 shall be returned and errno E_BADF set to indicate the
 * error.
 */
void sys_write(tf_t *tf)
{
    // int fd = syscall_get_arg2(tf);
    // unsigned int buf = syscall_get_arg3(tf);
    // size_t nbytes = syscall_get_arg4(tf);

    // unsigned int cur_pid = get_curid();

    // struct file **open_files = tcb_get_openfiles(cur_pid);

    // struct file *indexed_file = open_files[fd];

    // if (indexed_file == NULL || nbytes >= 10000)
    // {
    //     syscall_set_errno(tf, E_BADF);
    //     return;
    // }
}

/**
 * Return Value: Upon successful completion, 0 shall be returned; otherwise, -1
 * shall be returned and errno E_BADF set to indicate the error.
 */
void sys_close(tf_t *tf)
{
    // int fd = syscall_get_arg2(tf);

    // return;
}
// TODO

void sys_fstat(tf_t *tf)
{
    // TODO
    return;
}

/**
 * Create the path new as a link to the same inode as old.
 */
void sys_link(tf_t *tf)
{
    size_t oldlen = syscall_get_arg4(tf);
    size_t newlen = syscall_get_arg5(tf);
    if (oldlen > 128 || newlen > 128)
    {
        // TODO: set appropriate errno
        syscall_set_errno(tf, -1);
        return;
    }
    char name[DIRSIZ], new[newlen + 1], old[oldlen + 1];
    struct inode *dp, *ip;

    if (pt_copyin(get_curid(), syscall_get_arg2(tf), old, oldlen) != oldlen ||
        pt_copyin(get_curid(), syscall_get_arg3(tf), new, newlen) != newlen)
    {
        // TODO: set appropriate errno
        syscall_set_errno(tf, -1);
        return;
    }

    new[newlen] = '\0';
    old[oldlen] = '\0';

    if ((ip = namei(old)) == 0)
    {
        syscall_set_errno(tf, E_NEXIST);
        return;
    }

    begin_trans();

    inode_lock(ip);
    if (ip->type == T_DIR)
    {
        inode_unlockput(ip);
        commit_trans();
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }

    ip->nlink++;
    inode_update(ip);
    inode_unlock(ip);

    if ((dp = nameiparent(new, name)) == 0)
        goto bad;
    inode_lock(dp);
    if (dp->dev != ip->dev || dir_link(dp, name, ip->inum) < 0)
    {
        inode_unlockput(dp);
        goto bad;
    }
    inode_unlockput(dp);
    inode_put(ip);

    commit_trans();

    syscall_set_errno(tf, E_SUCC);
    return;

bad:
    inode_lock(ip);
    ip->nlink--;
    inode_update(ip);
    inode_unlockput(ip);
    commit_trans();
    syscall_set_errno(tf, E_DISK_OP);
    return;
}

/**
 * Is the directory dp empty except for "." and ".." ?
 */
static int isdirempty(struct inode *dp)
{
    int off;
    struct dirent de;

    for (off = 2 * sizeof(de); off < dp->size; off += sizeof(de))
    {
        if (inode_read(dp, (char *)&de, off, sizeof(de)) != sizeof(de))
            KERN_PANIC("isdirempty: readi");
        if (de.inum != 0)
            return 0;
    }
    return 1;
}

void sys_unlink(tf_t *tf)
{
    size_t len = syscall_get_arg3(tf);
    if (len > 128)
    {
        // TODO: Find the appropriate error number
        syscall_set_errno(tf, -1);
        return;
    }

    struct inode *ip, *dp;
    struct dirent de;
    char name[DIRSIZ], path[len + 1];
    uint32_t off;

    // check to see if copying is successful
    if (pt_copyin(get_curid(), syscall_get_arg2(tf), path, len) != len)
    {
        // TODO: set appropriate errno
        syscall_set_errno(tf, -1);
    };

    path[len] = '\0';

    if ((dp = nameiparent(path, name)) == 0)
    {
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }

    begin_trans();

    inode_lock(dp);

    // Cannot unlink "." or "..".
    if (dir_namecmp(name, ".") == 0 || dir_namecmp(name, "..") == 0)
        goto bad;

    if ((ip = dir_lookup(dp, name, &off)) == 0)
        goto bad;
    inode_lock(ip);

    if (ip->nlink < 1)
        KERN_PANIC("unlink: nlink < 1");
    if (ip->type == T_DIR && !isdirempty(ip))
    {
        inode_unlockput(ip);
        goto bad;
    }

    memset(&de, 0, sizeof(de));
    if (inode_write(dp, (char *)&de, off, sizeof(de)) != sizeof(de))
        KERN_PANIC("unlink: writei");
    if (ip->type == T_DIR)
    {
        dp->nlink--;
        inode_update(dp);
    }
    inode_unlockput(dp);

    ip->nlink--;
    inode_update(ip);
    inode_unlockput(ip);

    commit_trans();

    syscall_set_errno(tf, E_SUCC);
    return;

bad:
    inode_unlockput(dp);
    commit_trans();
    syscall_set_errno(tf, E_DISK_OP);
    return;
}

static struct inode *create(char *path, short type, short major, short minor)
{
    uint32_t off;
    struct inode *ip, *dp;
    char name[DIRSIZ];

    if ((dp = nameiparent(path, name)) == 0)
        return 0;
    inode_lock(dp);

    if ((ip = dir_lookup(dp, name, &off)) != 0)
    {
        inode_unlockput(dp);
        inode_lock(ip);
        if (type == T_FILE && ip->type == T_FILE)
            return ip;
        inode_unlockput(ip);
        return 0;
    }

    if ((ip = inode_alloc(dp->dev, type)) == 0)
        KERN_PANIC("create: ialloc");

    inode_lock(ip);
    ip->major = major;
    ip->minor = minor;
    ip->nlink = 1;
    inode_update(ip);

    if (type == T_DIR)
    {                // Create . and .. entries.
        dp->nlink++; // for ".."
        inode_update(dp);
        // No ip->nlink++ for ".": avoid cyclic ref count.
        if (dir_link(ip, ".", ip->inum) < 0 || dir_link(ip, "..", dp->inum) < 0)
            KERN_PANIC("create dots");
    }

    if (dir_link(dp, name, ip->inum) < 0)
        KERN_PANIC("create: dir_link");

    inode_unlockput(dp);
    return ip;
}

void sys_open(tf_t *tf)
{
    size_t len = syscall_get_arg4(tf);
    if (len > 128)
    {
        // TODO: Find the appropriate error number
        syscall_set_errno(tf, -1);
        return;
    }
    char path[len + 1];
    int fd, omode;
    struct file *f;
    struct inode *ip;

    if (pt_copyin(get_curid(), syscall_get_arg2(tf), path, len) != len)
    {
        // TODO: set appropriate errno
        syscall_set_errno(tf, -1);
        return;
    };

    path[len] = '\0';

    omode = syscall_get_arg3(tf);

    if (omode & O_CREATE)
    {
        begin_trans();
        ip = create(path, T_FILE, 0, 0);
        KERN_INFO("open: create status %p\n", ip);
        commit_trans();
        if (ip == 0)
        {
            syscall_set_retval1(tf, -1);
            syscall_set_errno(tf, E_CREATE);
            return;
        }
    }
    else
    {
        if ((ip = namei(path)) == 0)
        {
            syscall_set_retval1(tf, -1);
            syscall_set_errno(tf, E_NEXIST);
            return;
        }
        inode_lock(ip);
        if (ip->type == T_DIR && omode != O_RDONLY)
        {
            inode_unlockput(ip);
            syscall_set_retval1(tf, -1);
            syscall_set_errno(tf, E_DISK_OP);
            return;
        }
    }

    if ((f = file_alloc()) == 0 || (fd = fdalloc(f)) < 0)
    {
        if (f)
            file_close(f);
        inode_unlockput(ip);
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }
    inode_unlock(ip);

    f->type = FD_INODE;
    f->ip = ip;
    f->off = 0;
    f->readable = !(omode & O_WRONLY);
    f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
    syscall_set_retval1(tf, fd);
    syscall_set_errno(tf, E_SUCC);
}

void sys_mkdir(tf_t *tf)
{

    size_t len = syscall_get_arg3(tf);
    if (len > 128)
    {
        // TODO: Find the appropriate error number
        syscall_set_errno(tf, -1);
        return;
    }
    char path[len + 1];
    struct inode *ip;

    if (pt_copyin(get_curid(), syscall_get_arg2(tf), path, len) != len)
    {
        // TODO: set appropriate errno
        syscall_set_errno(tf, -1);
        return;
    };
    path[len] = '\0';

    begin_trans();
    if ((ip = (struct inode *)create(path, T_DIR, 0, 0)) == 0)
    {
        commit_trans();
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }
    inode_unlockput(ip);
    commit_trans();
    syscall_set_errno(tf, E_SUCC);
}

void sys_chdir(tf_t *tf)
{

    size_t len = syscall_get_arg3(tf);
    if (len > 128)
    {
        // TODO: Find the appropriate error number
        syscall_set_errno(tf, -1);
        return;
    }
    char path[len + 1];
    struct inode *ip;
    int pid = get_curid();

    if (pt_copyin(get_curid(), syscall_get_arg2(tf), path, len) != len)
    {
        // TODO: set appropriate errno
        syscall_set_errno(tf, -1);
        return;
    };

    path[len] = '\0';

    if ((ip = namei(path)) == 0)
    {
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }
    inode_lock(ip);
    if (ip->type != T_DIR)
    {
        inode_unlockput(ip);
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }
    inode_unlock(ip);
    inode_put(tcb_get_cwd(pid));
    tcb_set_cwd(pid, ip);
    syscall_set_errno(tf, E_SUCC);
    tcb_set_cwd(pid, ip);
}
