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
#include <kern/lib/spinlock.h>
#include "dir.h"
#include "path.h"
#include "file.h"
#include "fcntl.h"
#include "log.h"

// Kernel buffer does not need to be explicitly cleared/erased between uses.
// The required number of bytes at the beginning of the kernel buffer is simply overwritten
// and when data is read from the kernel buffer, only the same number of bytes is read.
// It is sufficient to ensure that the procedure writing to and reading from the kernel
// buffer is consecutive (i.e. atomic).
// We only need the kernel_buffer for write and read syscalls because the user data we read
// from and write to in those syscalls can be as large as 10000 bytes, which exceeds the
// kernel stack limit. For other functions like fstat, we can operate on kernel stack memory
// instead.
#define KERNEL_BUFFER_SIZE 10000
char kernel_buffer[KERNEL_BUFFER_SIZE];

spinlock_t kernel_buffer_lock;


void kernel_buffer_init(void)
{
    spinlock_init(&kernel_buffer_lock);
}

// TODO: remove this
extern struct
{
    spinlock_t lock;
    struct inode inode[NINODE];
} inode_cache;

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
    for (unsigned int fd = 0; fd < NOFILE; fd++)
    {
        // Available file descriptors will have NULL pointers
        if (open_files[fd] == NULL)
        {
            tcb_set_openfiles(get_curid(), fd, f);
            return fd;
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
    int fd = syscall_get_arg2(tf);
    uintptr_t buf = syscall_get_arg3(tf);
    int nbytes = syscall_get_arg4(tf);
    // KERN_INFO("sys_read fd=%d buf=%p nbytes=%d\n", fd, buf, nbytes);

    // Error checking
    if (fd >= NOFILE || fd < 0)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }
    // if not in file descriptpor table or the number of bytes exceeds
    if (nbytes >= KERNEL_BUFFER_SIZE)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }

    unsigned int cur_pid = get_curid();
    struct file **open_files = tcb_get_openfiles(cur_pid);
    struct file *f = open_files[fd];
    // KERN_INFO("sys_read inum=%d file size %d\n", f->ip->inum, f->ip->size);

    // More error checking
    if (f == NULL || f->readable == 0 || f->type != FD_INODE)
    {
        KERN_INFO("sys_read: file descriptor bad\n");
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }

    spinlock_acquire(&kernel_buffer_lock);
    int bytesReadFromFile;
    if ((bytesReadFromFile = file_read(f, kernel_buffer, nbytes)) == -1)
    {
        KERN_INFO("sys_read: file_read failed\n");
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        spinlock_release(&kernel_buffer_lock);
        return;
    }
    // KERN_INFO("sys_read: bytesReadFromFile=%d\n", bytesReadFromFile);
    // If bytesReadFromFile is positive, any errors handled by later if block
    // If bytesReadFromFile is negative (-1), we already caught the error previously
    // This block only fires if we are trying to read 0 bytes, we want this to go
    // through without an error.
    if (bytesReadFromFile == 0)
    {
        spinlock_release(&kernel_buffer_lock);
        syscall_set_retval1(tf, 0);
        syscall_set_errno(tf, E_SUCC);
        return;
    }
    size_t bytesCopiedToUser;
    if ((bytesCopiedToUser = pt_copyout(kernel_buffer, cur_pid, buf, bytesReadFromFile)) == 0)
    {
        KERN_INFO("sys_read: pt_copyout failed %d %d %d\n", bytesCopiedToUser, bytesReadFromFile, nbytes);
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        spinlock_release(&kernel_buffer_lock);
        return;
    }
    spinlock_release(&kernel_buffer_lock);
    // KERN_INFO("sys_read successful, bytesCopiedToUser=%d\n", bytesCopiedToUser);
    syscall_set_retval1(tf, bytesCopiedToUser);
    syscall_set_errno(tf, E_SUCC);
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
    int fd = syscall_get_arg2(tf);
    uintptr_t buf = syscall_get_arg3(tf);
    size_t nbytes = syscall_get_arg4(tf);

    // Error checking
    if (fd >= NOFILE || fd < 0)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }
    if (nbytes >= KERNEL_BUFFER_SIZE)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }

    unsigned int cur_pid = get_curid();
    struct file **open_files = tcb_get_openfiles(cur_pid);
    struct file *f = open_files[fd];
    // KERN_INFO("sys_write file size %d\n", f->ip->size);

    // More error checking
    if (f == NULL || f->writable == 0 || f->type != FD_INODE)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }

    // If we are trying to write 0 bytes, just do nothing and succeed. We cannot differentiate
    // writing zero bytes and an error in pt_copyin.
    if (nbytes == 0)
    {
        syscall_set_retval1(tf, 0);
        syscall_set_errno(tf, E_SUCC);
        return;
    }

    // Read user buffer into kernel buffer
    spinlock_acquire(&kernel_buffer_lock);
    int bytesCopiedFromUser;
    if ((bytesCopiedFromUser = pt_copyin(cur_pid, buf, kernel_buffer, nbytes)) == 0)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        spinlock_release(&kernel_buffer_lock);
        return;
    }
    // Write kernel buffer into file
    int bytesWrittenToFile;
    if ((bytesWrittenToFile = file_write(f, kernel_buffer, bytesCopiedFromUser)) == -1)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        spinlock_release(&kernel_buffer_lock);
        return;
    }
    spinlock_release(&kernel_buffer_lock);
    // KERN_INFO("sys_write: wrote %d bytes\n", bytesWrittenToFile);
    syscall_set_retval1(tf, bytesWrittenToFile);
    syscall_set_errno(tf, E_SUCC);
}

/**
 * Return Value: Upon successful completion, 0 shall be returned; otherwise, -1
 * shall be returned and errno E_BADF set to indicate the error.
 */
void sys_close(tf_t *tf)
{
    int fd = syscall_get_arg2(tf);

    // Error checking
    if (fd >= NOFILE || fd < 0)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }

    unsigned int cur_pid = get_curid();
    struct file **open_files = tcb_get_openfiles(cur_pid);
    struct file *f = open_files[fd];

    // More error checking
    if (f == NULL || f->type != FD_INODE)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }

    file_close(f);
    tcb_set_openfiles(cur_pid, fd, NULL);

    syscall_set_retval1(tf, 0);
    syscall_set_errno(tf, E_SUCC);
}

// Constructs and returns a struct file_stat for the specified fd
// Syscall has return value 0 if success, -1 in case of behavior
void sys_fstat(tf_t *tf)
{
    int fd = syscall_get_arg2(tf);
    uintptr_t fstatUser = syscall_get_arg3(tf);

    // Error checking
    if (fd >= NOFILE || fd < 0)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }

    unsigned int cur_pid = get_curid();
    struct file **open_files = tcb_get_openfiles(cur_pid);
    struct file *f = open_files[fd];

    // More error checking
    if (f == NULL)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }

    struct file_stat fs;
    fs.type = f->ip->type;
    fs.dev = f->ip->dev;
    fs.ino = f->ip->inum;
    fs.nlink = f->ip->nlink;
    fs.size = f->ip->size;

    if (pt_copyout(&fs, cur_pid, fstatUser, sizeof(struct file_stat)) == 0)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }

    syscall_set_retval1(tf, 0);
    syscall_set_errno(tf, E_SUCC);
}

/**
 * Create the path new as a link to the same inode as old.
 */
void sys_link(tf_t *tf)
{
    //    KERN_INFO("sys_link enter\n");
    size_t oldlen = syscall_get_arg4(tf);
    size_t newlen = syscall_get_arg5(tf);
    if (oldlen > 128 || newlen > 128)
    {
        //        KERN_INFO("sys_link len check failed\n");
        // TODO: set appropriate errno
        syscall_set_errno(tf, -1);
        return;
    }
    char name[DIRSIZ], new[newlen + 1], old[oldlen + 1];
    struct inode *dp, *ip;

    if (pt_copyin(get_curid(), syscall_get_arg2(tf), old, oldlen) != oldlen ||
        pt_copyin(get_curid(), syscall_get_arg3(tf), new, newlen) != newlen)
    {
        //        KERN_INFO("sys_link copyin failed\n");
        // TODO: set appropriate errno
        syscall_set_errno(tf, -1);
        return;
    }

    new[newlen] = '\0';
    old[oldlen] = '\0';

    if ((ip = namei(old)) == 0)
    {
        //        KERN_INFO("sys_link existing failed\n");
        syscall_set_errno(tf, E_NEXIST);
        return;
    }
    //    KERN_INFO("sys_link: before %d %d %d %d %d\n", ip->nlink, ip->inum, ip->type, ip->dev, ip->ref);

    begin_trans();

    inode_lock(ip);
    if (ip->type == T_DIR)
    {
        //        KERN_INFO("sys_link dir failed\n");
        inode_unlockput(ip);
        commit_trans();
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }

    ip->nlink++;
    inode_update(ip);
    inode_unlock(ip);

    if ((dp = nameiparent(new, name)) == 0)
    {
        //        KERN_INFO("sys_link namei failed\n");
        goto bad;
    }
    inode_lock(dp);
    if (dp->dev != ip->dev || dir_link(dp, name, ip->inum) < 0)
    {
        //        KERN_INFO("sys_link dev failed\n");
        inode_unlockput(dp);
        goto bad;
    }
    inode_unlockput(dp);
    inode_put(ip);

    commit_trans();
    //
    //    KERN_INFO("sys_link: after %d %d %d %d %d\n", ip->nlink, ip->inum, ip->type, ip->dev, ip->ref);
    //    KERN_INFO("sys_link success\n");
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

    //    KERN_INFO("sys_unlink: off %d\n", off);
    //    KERN_INFO("sys_unlink: before %d %d %d %d %d\n", ip->nlink, ip->inum, ip->type, ip->dev, ip->ref);
    //    struct dirent readDebug;
    //    inode_read(dp, (char *) &readDebug, off, sizeof(readDebug));
    //    KERN_INFO("sys_unlink: at offset %d %s\n", readDebug.inum, readDebug.name);

    if (ip->nlink < 1)
        KERN_PANIC("unlink: nlink < 1");
    if (ip->type == T_DIR && !isdirempty(ip))
    {
        inode_unlockput(ip);
        goto bad;
    }

    //    KERN_INFO("sys_unlink %d %s\n", ip->inum, de.name);

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
    //    KERN_INFO("sys_unlink: after %d %d %d %d %d\n", ip->nlink, ip->inum, ip->type, ip->dev, ip->ref);

    //    KERN_INFO("sys_unlink success\n");
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
        syscall_set_errno(tf, E_INVAL_ARG);
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

    // ip is the inode that points to the file being opened
    if (omode & O_CREATE)
    {
        begin_trans();
        ip = create(path, T_FILE, 0, 0);
        // KERN_INFO("open: create status %p\n", ip);
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
        //        KERN_INFO("sys_open obtained ip %d type %d\n", ip->inum, ip->type);
        inode_lock(ip);
        if (ip->type == T_DIR && omode != O_RDONLY)
        {
            inode_unlockput(ip);
            syscall_set_retval1(tf, -1);
            syscall_set_errno(tf, E_DISK_OP);
            return;
        }
    }

    // file descriptor array is an array of struct file pointers
    // we find an empty index in the file d array
    // we allocate a struct file
    // then we point the fd array entry to the file struct
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

    // initialize fields of file struct
    f->type = FD_INODE;
    f->ip = ip;
    f->off = 0;
    f->readable = !(omode & O_WRONLY);
    f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

    // KERN_INFO("sys_open inum=%d\n", ip->inum);
    syscall_set_retval1(tf, fd);

    // TODO: remove this
    // for (size_t i = 0; i < 5; i++) {
    //     KERN_INFO("open end inode_cache inum=%d type=%d\n", inode_cache.inode[i].inum, inode_cache.inode[i].type);
    // }

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
    // KERN_INFO("mkdir %s inum=%d type=%d\n", path, ip->inum, ip->type);
    inode_unlockput(ip);
    commit_trans();
    syscall_set_errno(tf, E_SUCC);
}

void sys_chdir(tf_t *tf)
{
    // TODO: REMOVE THIS
    // for (size_t i = 0; i < 5; i++) {
    //     KERN_INFO("chdir start inode_cache inum=%d type=%d\n", inode_cache.inode[i].inum, inode_cache.inode[i].type);
    // }

    size_t len = syscall_get_arg3(tf);
    if (len > 128)
    {
        // TODO: Find the appropriate error number
        syscall_set_errno(tf, -1);
        return;
    }
    char path[len + 1];
    struct inode *ip;
    unsigned int pid = get_curid();

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

    // TODO: REMOVE THIS
    // for (size_t i = 0; i < 5; i++) {
    //     KERN_INFO("chdir end inode_cache inum=%d type=%d\n", inode_cache.inode[i].inum, inode_cache.inode[i].type);
    // }

    syscall_set_errno(tf, E_SUCC);
}

void sys_getcwd(tf_t *tf)
{
    // TODO: REMOVE THIS
    // for (size_t i = 0; i < 5; i++) {
    //     KERN_INFO("getcwd start inode_cache inum=%d type=%d ref=%d busy=%d\n", inode_cache.inode[i].inum, inode_cache.inode[i].type, inode_cache.inode[i].ref, inode_cache.inode[i].flags & I_BUSY);
    // }

    //    uintptr_t userBuffer = syscall_get_arg2(tf);    // User buffer must be at least MAX_CWD_LEN bytes
    unsigned int pid = get_curid();

    //    KERN_INFO("sys_getcwd enter\n");
    spinlock_acquire(&kernel_buffer_lock);
    char *reversedPath = kernel_buffer;
    reversedPath[0] = '\0';
    size_t reversedPath_i = 1; // next available index (i.e. length)

    //    KERN_INFO("sys_getcwd: allocated\n");

    struct inode *leaf = tcb_get_cwd(pid); // leaf must be a directory
    inode_dup(leaf);
    inode_lock(leaf);
    // / a b c d
    // 2 0 0 0 1
    // * * - - -

    // /
    // 1
    // -

    // / a
    // 1 1
    // * -

    // / a b
    // 64 0 1
    // - - -
    //    KERN_INFO("sys_getcwd: leaf=%p inum=%d\n", leaf, leaf->inum);
    while (leaf->inum != ROOTINO)
    {
        //        KERN_INFO("sys_getcwd: looking at leaf=%d\n", leaf->inum);
        //        struct inode *parent = dir_lookup(leaf, "..", NULL);
        //        inode_lock(parent);
        struct inode *parent = dir_lookup(leaf, "..", NULL);
        inode_lock(parent);

        // TODO: REMOVE THIS
        // KERN_INFO("getcwd parent of %d is %d\n", leaf->inum, parent->inum);
        // for (size_t i = 0; i < 5; i++) {
        //     KERN_INFO("getcwd end inode_cache inum=%d type=%d ref=%d busy=%d\n", inode_cache.inode[i].inum, inode_cache.inode[i].type, inode_cache.inode[i].ref, inode_cache.inode[i].flags & I_BUSY);
        // }

        for (unsigned int offset = 0; offset < parent->size; offset += sizeof(struct dirent))
        {
            struct dirent sub_dir;
            inode_read(parent, (char *)(&sub_dir), offset, sizeof(struct dirent));
            if (sub_dir.inum == leaf->inum)
            {
                char *subdirName = sub_dir.name; // Found name of leaf directory
                int lastChar_i = DIRSIZ - 1;     // index of last char in subdirName (char before null term)
                for (int i = 0; i < DIRSIZ; i++)
                {
                    if (subdirName[i] == '\0')
                    {
                        lastChar_i = i - 1;
                        break;
                    }
                }

                // slash at the end of each directory name
                if (reversedPath_i >= KERNEL_BUFFER_SIZE)
                {
                    // Path length is greater than MAX_CWD_LEN
                    syscall_set_errno(tf, -1);
                    syscall_set_retval1(tf, -1);
                    spinlock_release(&kernel_buffer_lock);
                    return;
                }
                reversedPath[reversedPath_i++] = '/';

                for (int i = lastChar_i; i >= 0; i--)
                {
                    if (reversedPath_i >= KERNEL_BUFFER_SIZE)
                    {
                        // Path length is greater than MAX_CWD_LEN
                        syscall_set_errno(tf, -1);
                        syscall_set_retval1(tf, -1);
                        spinlock_release(&kernel_buffer_lock);
                        return;
                    }
                    reversedPath[reversedPath_i++] = subdirName[i];
                }
                break;
            }
        }
        inode_unlockput(leaf);
        leaf = parent;
    }
    inode_unlockput(leaf);


    // top-level root slash
    if (reversedPath_i >= KERNEL_BUFFER_SIZE)
    {
        // Path length is greater than MAX_CWD_LEN
        syscall_set_errno(tf, -1);
        syscall_set_retval1(tf, -1);
        spinlock_release(&kernel_buffer_lock);
        return;
    }
    reversedPath[reversedPath_i++] = '/';
    //    KERN_INFO("sys_getcwd: final reversedPath_i=%d\n", reversedPath_i);

    // reverse path
    for (int i = 0; i < reversedPath_i / 2; i++)
    {
        /*
         * 12345678
         * 82345671
         * 87235621, etc
         * 12345
         * 52341
         * 54321 (no need to swap middle elem)
         */
        char tmp = reversedPath[i];
        reversedPath[i] = reversedPath[reversedPath_i - i - 1];
        reversedPath[reversedPath_i - i - 1] = tmp;
    }

     KERN_INFO("%s\n", reversedPath);
    //    if (pt_copyout(reversedPath, pid, (uintptr_t)kernel_buffer, reversedPath_i) != reversedPath_i) {
    //        syscall_set_errno(tf, -1);
    //        syscall_set_retval1(tf, -1);
    //        spinlock_release(&kernel_buffer_lock);
    //        return;
    //    }

    spinlock_release(&kernel_buffer_lock);
    syscall_set_retval1(tf, reversedPath_i);
    syscall_set_errno(tf, E_SUCC);
}


void sys_rename(tf_t * tf) {
    size_t len_src = syscall_get_arg4(tf);
    size_t len_dest = syscall_get_arg5(tf);
    if (len_src > 128 || len_dest > 128) {
        syscall_set_errno(tf, -1);
        return;
    }
    char path_dest[len_dest + 1];
    char path_src[len_src + 1];

    if (pt_copyin(get_curid(), syscall_get_arg2(tf), path_src, len_src) != len_src ||
        pt_copyin(get_curid(), syscall_get_arg3(tf), path_dest, len_dest) != len_dest) {
        // TODO: set appropriate errno
        syscall_set_errno(tf, -1);
        return;
    }
    path_dest[len_dest] = '\0';
    path_src[len_src] = '\0';

//    KERN_INFO("sys_rename: %s -> %s\n", path_src, path_dest);

    char name_src[DIRSIZ];
    char name_dest[DIRSIZ];
    struct inode *dp_src;
    struct inode *dp_dest;
    struct inode *ip_src;
    uint32_t off_src;

    begin_trans();

    if((dp_src = nameiparent(path_src, name_src)) == 0) {
//        KERN_INFO("sys_rename: src parent does not exist\n");
        commit_trans();
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }
    if((dp_dest = nameiparent(path_dest, name_dest)) == 0) {
//        KERN_INFO("sys_rename: dst parent does not exist\n");
        inode_put(dp_src);
        commit_trans();
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }

    if (dir_namecmp(name_src, ".") == 0 || dir_namecmp(name_src, "..") == 0 ||
        dir_namecmp(name_dest, ".") == 0 || dir_namecmp(name_dest, "..") == 0) {
        KERN_INFO("mv: cannot find move . or ..\n", name_src);
        if (dp_src == dp_dest) {
            inode_put(dp_src);
        } else {
            inode_put(dp_src);
            inode_put(dp_dest);
        }
        commit_trans();
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }

    if (dp_src == dp_dest) {
//        KERN_INFO("sys_rename: src and dest have same parent\n");
        inode_put(dp_dest);
        inode_lock(dp_src);
    } else {
//        KERN_INFO("sys_rename: src and dest don't have the same parent\n");
        inode_lock(dp_src);
        inode_lock(dp_dest);
    }

    ip_src = dir_lookup(dp_src, name_src, &off_src);
    if (ip_src == 0) {
        // path_src is invalid, leaf does not exist
//        KERN_INFO("sys_rename: cannot find %s dirent entry\n", name_src);
        if (dp_src == dp_dest) {
            inode_unlockput(dp_src);
        } else {
            inode_unlockput(dp_src);
            inode_unlockput(dp_dest);
        }
        commit_trans();
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }

    inode_lock(ip_src);
    if (ip_src->dev != dp_src->dev || ip_src->dev != dp_dest->dev) {
        // mv src dest not on the same device
//        KERN_INFO("sys_rename: ip_src dev %d, dp_src %d, dp_dest %d\n", ip_src->dev, dp_src->dev, dp_dest->dev);
        inode_unlockput(ip_src);
        if (dp_src == dp_dest) {
            inode_unlockput(dp_src);
        } else {
            inode_unlockput(dp_src);
            inode_unlockput(dp_dest);
        }
        commit_trans();
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }
    if (ip_src->nlink < 1) {
//        KERN_INFO("sys_rename: nlnk <1 %d\n", ip_src->dev, dp_src->dev, dp_dest->dev);
        inode_unlockput(ip_src);
        if (dp_src == dp_dest) {
            inode_unlockput(dp_src);
        } else {
            inode_unlockput(dp_src);
            inode_unlockput(dp_dest);
        }
        commit_trans();
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }
    if (dir_link(dp_dest, name_dest, ip_src->inum) < 0){
        // linking fails
//        KERN_INFO("sys_rename linking fails\n");
        inode_unlockput(ip_src);
        if(dp_src == dp_dest){
            inode_unlockput(dp_src);
        }
        else{
            inode_unlockput(dp_src);
            inode_unlockput(dp_dest);
        }
        commit_trans();
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }
    struct dirent zero;
    memset(&zero, 0, sizeof(struct dirent));
    if (inode_write(dp_src, (char *) &zero, off_src, sizeof(struct dirent)) != sizeof(struct dirent)) {
        // unlinking failed
//        KERN_PANIC("rename: writei");
    }
    if (ip_src->type == T_DIR) {
//        KERN_INFO("sys_rename: old nlinks src %d dest %d\n", dp_src->nlink, dp_dest->nlink);
        dp_src->nlink--;
        dp_dest->nlink++;
//        KERN_INFO("sys_rename: new nlinks src %d dest %d\n", dp_src->nlink, dp_dest->nlink);
        inode_update(dp_src);
        inode_update(dp_dest);
    }

    inode_unlockput(ip_src);
    if(dp_src == dp_dest){
        inode_unlockput(dp_src);
    }
    else{
        inode_unlockput(dp_src);
        inode_unlockput(dp_dest);
    }
    commit_trans();
//    KERN_INFO("sys_rename success\n");
    syscall_set_errno(tf, E_SUCC);
    return;
}