#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <x86.h>
#include <file.h>
#include <gcc.h>

#define exit(...) return __VA_ARGS__
#define T_DIR 1           // Directory
#define T_FILE 2          // File
#define T_DEV 3           // Device
#define IO_CHUNK_SIZE 512 // Reading and writing in chunks of 512 bytes

// Definition of dirent copied over from kernel, needed by ls to iterate over dirents
#define DIRSIZ 14
struct dirent
{
    uint16_t inum;
    char name[DIRSIZ]; // name should always include the null terminator
};

void shell_ls(char *relativePath)
{
    int dirfd;
    size_t nDirents;
    struct file_stat stat;

    dirfd = open(relativePath, O_RDONLY);
    if (dirfd < 0)
    {
        printf("ls: no such file or directory: %s\n", relativePath);
        return;
    }
    int fstat_status = fstat(dirfd, &stat);
    ASSERT(fstat_status == 0);

    if (stat.type != T_DIR)
    {
        printf("ls: %s is not a directory\n", relativePath);
        return;
    }
    // fstat, dirfd, directory checking
    ASSERT(stat.size % sizeof(struct dirent) == 0);
    nDirents = stat.size / sizeof(struct dirent);
    for (size_t i = 0; i < nDirents; i++)
    {
        struct dirent cur_dirent;

        read(dirfd, (char *)&cur_dirent, sizeof(struct dirent));
        if (cur_dirent.inum == 0) // inum 0 indicates empty entry
            continue;
        printf("%s\t", cur_dirent.name);
    }
    printf("\n");
    close(dirfd);
}

void shell_mkdir(char *name)
{
    int dirfd = open(name, O_RDONLY);
    if (dirfd >= 0)
    {
        ASSERT(close(dirfd) == 0);
        printf("mkdir: %s already exists\n", name);
        return;
    }
    int mkdirStatus = mkdir(name);
    ASSERT(mkdirStatus == 0);
}

void shell_cd(char *relativePath)
{
    if (chdir(relativePath) != 0)
    {
        printf("cd: no such directory: %s\n", relativePath);
        return;
    }
}

void shell_touch(char *relativePath)
{
    int fd = open(relativePath, O_CREATE);
    if (fd >= 0)
    {
        close(fd);
    }
    else
    {
        printf("touch: unable to create file: %s\n", relativePath);
    }
}

void shell_cat(char *relativePath)
{
    struct file_stat stat;

    int fd = open(relativePath, O_RDONLY);
    if (fd < 0)
    {
        printf("cat: no such file: %s\n", relativePath);
        return;
    }
    int fstat_status = fstat(fd, &stat);
    ASSERT(fstat_status == 0);

    if (stat.type != T_FILE)
    {
        printf("cat: %s: is a directory\n", relativePath);
        ASSERT(close(fd) == 0);
        return;
    }

    char buffer[IO_CHUNK_SIZE + 1];
    size_t readsize;
    while ((readsize = read(fd, buffer, IO_CHUNK_SIZE)) > 0)
    {
        buffer[readsize] = '\0';
        printf("%s", buffer);
    }
    if (readsize == -1)
    {
        printf("cat:read failed\n");
        close(fd);
        return;
    }
    close(fd);
}

void shell_mv(char *src, char *dst)
{
    // NOTE: current mv syntax is NOT regular shell syntax, second argument must be file path
    int src_fd;
    int dst_fd;
    //    int src_fstat_status;
    //    printf("shell_mv: enter(src=%s, dst=%s)\n", src, dst);

    src_fd = open(src, O_RDONLY);
    if (src_fd < 0)
    {
        printf("mv: no such file or directory: %s\n", src);
        return;
    }
    //    struct file_stat source_stat;
    //    src_fstat_status = fstat(src_fd, &source_stat);
    //    ASSERT(src_fstat_status == 0);
    //    if(source_stat.type != T_FILE){
    //        printf("mv: source %s is not a file\n", src);
    //        close(src_fd);
    //        return;
    //    }

    dst_fd = open(dst, O_RDONLY);
    if (dst_fd >= 0)
    {
        printf("mv: destination already exists: %s\n", dst);
        close(src_fd);
        close(dst_fd);
        return;
    }
    close(src_fd);

    if (rename(src, dst) != 0)
    {
        printf("mv: cannot move (nothing moved)\n");
        return;
    }
}

/*
 * If relativePath specifies a file, unlink the file
 * If relativePath specifies a directory, recursively unlink its contents and the directory
 * Returns 0 on success, -1 on failure
 */
int remove(char *relativePath)
{
    int fd_root = open(relativePath, O_RDONLY);

    ASSERT(fd_root >= 0);

    struct file_stat fstat_root;
    int fstat_status = fstat(fd_root, &fstat_root);

    ASSERT(fstat_status == 0);
    ASSERT(fstat_root.type == T_DIR || fstat_root.type == T_FILE);

    if (fstat_root.type == T_FILE)
    {
        close(fd_root);
        int unlinkStatus = unlink(relativePath);

        ASSERT(unlinkStatus == 0);
        //        printf("remove: removed file %s\n", relativePath);
    }
    else
    {
        int chdir_status = chdir(relativePath);
        //        printf("remove: removing dir %s\n", relativePath);

        ASSERT(chdir_status == 0);
        ASSERT(fstat_root.size % sizeof(struct dirent) == 0);

        size_t nDirents = fstat_root.size / sizeof(struct dirent);
        for (size_t i = 0; i < nDirents; i++)
        {
            struct dirent cur_dirent;

            int read_status = read(fd_root, (char *)&cur_dirent, sizeof(struct dirent));
            ASSERT(read_status > 0);
            if (cur_dirent.inum == 0) // inum 0 indicates empty entry
                break;
            if (strcmp(cur_dirent.name, ".") == 0 || strcmp(cur_dirent.name, "..") == 0)
                continue;
            //            printf("remove: recursively removing %s\n", cur_dirent.name);
            int recursive_remove_status = remove(cur_dirent.name);
            ASSERT(recursive_remove_status == 0);
        }

        chdir_status = chdir("..");

        ASSERT(chdir_status == 0);

        close(fd_root);
        int unlinkStatus = unlink(relativePath);

        ASSERT(unlinkStatus == 0);
        //        printf("remove: removed dir %s\n", relativePath);
    }
    return 0;
}

void shell_rm(char *relativePath)
{
    int fd = open(relativePath, O_RDONLY);
    if (fd < 0)
    {
        printf("rm: no such file or directory: %s\n", relativePath);
        return;
    }
    struct file_stat stat;
    int fstat_status = fstat(fd, &stat);
    if (fstat_status != 0)
    {
        printf("rm: no such file or directory: %s\n", relativePath);
        close(fd);
        return;
    }
    if (stat.type != T_FILE)
    {
        close(fd);
        printf("rm: not a file: %s\n", relativePath);
        return;
    }
    int close_status = close(fd);
    ASSERT(close_status == 0);
    int remove_status = remove(relativePath);
    ASSERT(remove_status == 0);
}

void shell_rm_r(char *relativePath)
{
    int fd = open(relativePath, O_RDONLY);
    if (fd < 0)
    {
        printf("rm: no such file or directory: %s\n", relativePath);
        return;
    }
    struct file_stat stat;
    int fstat_status = fstat(fd, &stat);
    if (fstat_status != 0)
    {
        close(fd);
        printf("rm: no such file or directory: %s\n", relativePath);
        return;
    }
    if (stat.type != T_DIR)
    {
        close(fd);
        printf("rm: not a directory: %s\n", relativePath);
        return;
    }
    if (strcmp(relativePath, ".") == 0 || strcmp(relativePath, "..") == 0)
    {
        close(fd);
        printf("rm: cannot remove %s\n", relativePath);
        return;
    }
    int close_status = close(fd);
    ASSERT(close_status == 0);
    int remove_status = remove(relativePath);
    ASSERT(remove_status == 0);
}

/*
 * copy - dest doesn't exist
 * - create new file and write to it
 * copy - dest exists and is a file
 * - delete the old file, create new file and write to it
 * copy - dest exists and is a folder
 * - call copy on new folder
 * copy r - dest doesn't exist
 * - mkdir, call copy on each directory entry
 * copy r - dest exists
 * - call copy on each directory entry
/*
1. Non-recursive case
If src does not exist, return error.
If src is a directory, return error.
Try to open dst with O_CREATE.
  If dst exists and is a file, it will be overwritten;
  If dst does not exist, but its parent directory exists, then O_CREATE will create the dst for us;
  If dst exists and is a directory, create new file under dst with same name as src;
  If dst does not exist and O_CREATE cannot create the file, return error.

2. Recursive case
If src does not exist, return error.
If src is not a directory, return error.
Try to open dst.
  If dst exists and is a file, return error;
  If dst exists and is a directory, copy files and subdirectories under src into dst;
  If dst does not exist, try to create it with mkdir. If this fails, return error, otherwise proceed a
 */
void shell_copy_file(char *src, char *dest)
{ // cp
    int fd_src = open(src, O_RDONLY);
    if (fd_src < 0)
    {
        printf("cp: file does not exist: %s\n", src);
        return;
    }
    struct file_stat stat_src;
    if (fstat(fd_src, &stat_src) != 0)
    {
        printf("cp: cannot read file: %s\n", src);
        return;
    }
    if (stat_src.type != T_FILE)
    {
        printf("cp: not a file: %s\n", src);
        return;
    }

    int fd_dest = open(dest, O_RDWR | O_CREATE);
    char buffer[IO_CHUNK_SIZE];
    size_t bytesRead = 0;
    while ((bytesRead = read(fd_src, buffer, IO_CHUNK_SIZE)) > 0)
    {
        if (write(fd_dest, buffer, bytesRead) <= 0)
        {
            printf("cp: cannot write\n");
            return;
        }
    }
}
/*
cp -r a b
a/hello
b/a/hello/c

b/a/hello/c
b/a/hello/hello

b/a/hello/c
b/hello
 */
void shell_copy_folder(char *src, char *dest)
{ // cp -r
    int fd_src = open(src, O_RDONLY);
    if (fd_src < 0)
    {
        printf("cp: no such file or directory: %s\n", src);
    }

    struct file_stat stat_src;
    int fstat_status = fstat(fd_src, &stat_src);
    if (fstat_status != 0)
    {
        close(fd_src);
        printf("cp: no such file or directory: %s\n", src);
        return;
    }
    if (stat_src.type != T_DIR)
    {
        close(fd_src);
        printf("cp: not a directory: %s\n", src);
        return;
    }

    ASSERT(close(fd_src) == 0);
    int fd_dest = open(dest, O_RDONLY);
    // if the destination directory already exists
    if (fd_dest >= 0)
    {
    }
    else
    {
        ASSERT(mkdir(dest) == 0);
    }
}

void copy(char *src, char *dest)
{
    int fd_src = open(src, O_RDONLY);
    ASSERT(fd_src >= 0);
    struct file_stat fstat_src;
    ASSERT(fstat(fd_src, &fstat_src) == 0);
    ASSERT(fstat_src.type == T_FILE || fstat_src.type == T_DIR);
    if (fstat_src.type == T_FILE)
    {
        printf("src, dest: %s, %s\n",src, dest);
        char actual_dest[strlen(src) + strlen(dest) + 2];
        strcpy(actual_dest, dest);
        int fd_dest = open(dest, O_RDONLY);
        // if the destination file already exists
        if (fd_dest >= 0)
        {
            struct file_stat stat_dest;
            fstat(fd_dest, &stat_dest);
            if (stat_dest.type == T_DIR)
            {
                actual_dest[strlen(dest)] = '/';
                strcpy(&actual_dest[strlen(dest) + 1], src);
                actual_dest[strlen(dest) + strlen(src) + 1] = '\0';
            }
        }
        
        remove(dest);
        fd_dest = open(actual_dest, O_RDWR | O_CREATE);
        if (fd_dest < 0)
        {
            printf("cp: failed to create file %s\n", dest);
            return;
        }
        char buffer[IO_CHUNK_SIZE];
        size_t bytesRead = 0;
        while ((bytesRead = read(fd_src, buffer, IO_CHUNK_SIZE)) > 0)
        {
            if (write(fd_dest, buffer, bytesRead) <= 0)
            {
                printf("cp: failed to write\n");
                return;
            }
        }
        close(fd_src);
        close(fd_dest);
    }
    else
    {
        int fd_dest = open(dest, O_RDONLY);
        if (fd_dest >= 0)
        {
            struct file_stat stat_dest;
            fstat(fd_dest, &stat_dest);
            if (stat_dest.type == T_FILE)
            {
                printf("cp: is a file: %s\n", dest);
                return;
            }
            else if (stat_dest.type == T_DIR)
            {
                // DO NOTHING!
            }
            else
            {
                printf("cp: is not a file or directory: %s\n", dest);
                return;
            }
            close(fd_dest);
        }
        else
        {
            if (mkdir(dest) != 0)
            {
                printf("cp: cannot create dir: %s\n", dest);
                return; 
            }
        }
        if (fstat_src.size % sizeof(struct dirent) != 0)
        {
            printf("cp: src directory is corrupt: %s\n", src);
            return;
        }
        size_t nDirents = fstat_src.size / sizeof(struct dirent);
        for (size_t i = 0; i < nDirents; i++)
        {
            struct dirent cur_dirent;
            if(read(fd_src, (char *)&cur_dirent, sizeof(struct dirent)) <= 0){
                printf("cp: read fail");
            };
            if (cur_dirent.inum == 0) // inum 0 indicates empty entry
                break;
            if (strcmp(cur_dirent.name, ".") == 0 || strcmp(cur_dirent.name, "..") == 0)
                continue;
            size_t len_srcPath = strlen(src);
            size_t len_destPath = strlen(dest);
            char childSrcPath[len_srcPath + DIRSIZ + 1];
            char childDestPath[len_destPath + DIRSIZ + 1];
            strcpy(childSrcPath, src);
            strcpy(childDestPath, dest);
            childSrcPath[len_srcPath] = '/';
            childDestPath[len_destPath] = '/';
            strncpy(&childSrcPath[len_srcPath + 1], cur_dirent.name, DIRSIZ);
            strncpy(&childDestPath[len_destPath + 1], cur_dirent.name, DIRSIZ);
            copy(childSrcPath, childDestPath);
        }
        close(fd_src);
    }
}
void copy_old(char *srcPath, char *destPath)
{
    int fd_src = open(srcPath, O_RDONLY);
    int fd_dest = open(destPath, O_RDONLY);
    ASSERT(fd_src >= 0);
    ASSERT(fd_dest < 0);
    struct file_stat fstat_src;
    ASSERT(fstat(fd_src, &fstat_src) == 0);
    ASSERT(fstat_src.type == T_FILE || fstat_src.type == T_DIR);
    if (fstat_src.type == T_FILE)
    {
        fd_dest = open(destPath, O_RDWR | O_CREATE);
        ASSERT(fd_dest >= 0);
        char buffer[IO_CHUNK_SIZE];
        size_t bytesRead = 0;
        while ((bytesRead = read(fd_src, buffer, IO_CHUNK_SIZE)) > 0)
        {
            ASSERT(write(fd_dest, buffer, bytesRead) > 0);
        }
        ASSERT(close(fd_src) == 0);
        ASSERT(close(fd_dest) == 0);
        //        printf("copy: copied %s to %s\n", srcPath, destPath);
    }
    else
    {
        //        printf("copy: copying dir %s to %s\n", srcPath, destPath);
        ASSERT(mkdir(destPath) == 0);
        ASSERT(fstat_src.size % sizeof(struct dirent) == 0);
        size_t nDirents = fstat_src.size / sizeof(struct dirent);
        for (size_t i = 0; i < nDirents; i++)
        {
            struct dirent cur_dirent;

            ASSERT(read(fd_src, (char *)&cur_dirent, sizeof(struct dirent)) > 0);
            if (cur_dirent.inum == 0) // inum 0 indicates empty entry
                break;
            if (strcmp(cur_dirent.name, ".") == 0 || strcmp(cur_dirent.name, "..") == 0)
                continue;
            size_t len_srcPath = strlen(srcPath);
            size_t len_destPath = strlen(destPath);
            char childSrcPath[len_srcPath + DIRSIZ + 1];
            char childDestPath[len_destPath + DIRSIZ + 1];
            strcpy(childSrcPath, srcPath);
            strcpy(childDestPath, destPath);
            childSrcPath[len_srcPath] = '/';
            childDestPath[len_destPath] = '/';
            strncpy(&childSrcPath[len_srcPath + 1], cur_dirent.name, DIRSIZ);
            strncpy(&childDestPath[len_destPath + 1], cur_dirent.name, DIRSIZ);
            //            printf("copy: recursively copying %s to %s\n", childSrcPath, childDestPath);
            copy(childSrcPath, childDestPath);
        }
        ASSERT(close(fd_src) == 0);
    }
}

void shell_cp(char *srcPath, char *destPath)
{
    int fd_src = open(srcPath, O_RDONLY);
    if (fd_src < 0)
    {
        printf("cp: no such file or directory: %s\n", srcPath);
        return;
    }
    struct file_stat stat_src;
    int fstat_status = fstat(fd_src, &stat_src);
    if (fstat_status != 0)
    {
        close(fd_src);
        printf("cp: no such file or directory: %s\n", srcPath);
        return;
    }
    if (stat_src.type != T_FILE)
    {
        close(fd_src);
        printf("cp: not a file: %s\n", srcPath);
        return;
    }


    ASSERT(close(fd_src) == 0);
    // printf("hello world\n");
    copy(srcPath, destPath);
}

/*
 * cp hello hello2  hello -> hello2
 * cp hello hello2  hello -> hello2/hello
 * cp hello .       hello -> hello
 * hello/file1
 * hello/file2
 *
 *
 * hello2/hello
 * hello2/hello/file1
 * hello2/hello/test
 *
 * cp hello hello2
 */
void shell_cp_r(char *srcPath, char *destPath)
{
    int fd_src = open(srcPath, O_RDONLY);
    if (fd_src < 0)
    {
        printf("cp: no such file or directory: %s\n", srcPath);
        return;
    }
    struct file_stat stat_src;
    int fstat_status = fstat(fd_src, &stat_src);
    if (fstat_status != 0)
    {
        close(fd_src);
        printf("cp: no such file or directory: %s\n", srcPath);
        return;
    }
    if (stat_src.type != T_DIR)
    {
        close(fd_src);
        printf("cp: not a directory: %s\n", srcPath);
        return;
    }
    ASSERT(close(fd_src) == 0);
    // int fd_dest = open(destPath, O_RDONLY);
    // if (fd_dest >= 0) {
    //     ASSERT(close(fd_dest) == 0);
    //     printf("cp: %s already exists (not copied)\n", destPath);
    //     return;
    // }
    copy(srcPath, destPath);
}

void shell_echo(char *text, char *relativePath)
{
    int fd = open(relativePath, O_RDONLY);
    if (fd >= 0)
    {
        // If the file already exists, first check that it is not a directory
        // then remove it in order to wipe out its contents
        struct file_stat stat;
        int fstat_status = fstat(fd, &stat);
        ASSERT(fstat_status == 0);
        if (stat.type != T_FILE)
        {
            printf("echo: not a file: %s\n", relativePath);
            ASSERT(close(fd) == 0);
            return;
        }
        ASSERT(close(fd) == 0);
        ASSERT(remove(relativePath) == 0);
    }
    fd = open(relativePath, O_RDWR | O_CREATE);
    if (fd < 0)
    {
        printf("echo: no such file: %s\n", relativePath);
        return;
    }
    size_t len_text = strlen(text);
    for (size_t i = 0; i < len_text; i += IO_CHUNK_SIZE)
    {
        int writeStatus = write(fd, &text[i], MIN(len_text - i, IO_CHUNK_SIZE));
        if (writeStatus < 0)
        {
            printf("echo: write failed\n");
            ASSERT(close(fd) == 0);
            return;
        }
    }
    int writeStatus = write(fd, "\n", 1);
    if (writeStatus < 1)
    {
        printf("echo: write failed\n");
        ASSERT(close(fd) == 0);
        return;
    }
    ASSERT(close(fd) == 0);
}

void shell_echo_append(char *text, char *relativePath)
{
    int fd = open(relativePath, O_RDONLY);
    if (fd >= 0)
    {
        // If the file already exists, check that it is not a directory
        struct file_stat stat;
        int fstat_status = fstat(fd, &stat);
        ASSERT(fstat_status == 0);
        if (stat.type != T_FILE)
        {
            printf("echo: not a file: %s\n", relativePath);
            ASSERT(close(fd) == 0);
            return;
        }
        ASSERT(close(fd) == 0);
    }
    fd = open(relativePath, O_RDWR | O_CREATE);
    if (fd < 0)
    {
        printf("echo: no such file: %s\n", relativePath);
        return;
    }
    char unused_buffer[IO_CHUNK_SIZE]; // used to advance the file pointer to the end of the file
    while (read(fd, unused_buffer, IO_CHUNK_SIZE) > 0)
    {
    }

    size_t len_text = strlen(text);
    for (size_t i = 0; i < len_text; i += IO_CHUNK_SIZE)
    {
        int writeStatus = write(fd, &text[i], MIN(len_text - i, IO_CHUNK_SIZE));
        if (writeStatus < 0)
        {
            printf("echo: write failed\n");
            close(fd);
            return;
        }
    }
    int writeStatus = write(fd, "\n", 1);
    if (writeStatus < 1)
    {
        printf("echo: write failed\n");
        close(fd);
        return;
    }
    close(fd);
}

void shell_pwd()
{
    if (getcwd() == -1)
    {
        printf("pwd failed\n");
    }
}

typedef struct
{
    int found;
    const char *start;
    const char *nextStart;
    size_t len;
} argument;

argument findNextArg(const char *command)
{
    //    printf("find %s\n", command);
    argument r;
    size_t cur = 0;
    char endChar = ' ';
    int insideQuoteEmpty = 1;
    while (command[cur] == ' ' || command[cur] == 127)
    {
        cur++;
    }
    if (command[cur] == '"')
    {
        cur++;
        endChar = '"';
    }
    else if (command[cur] == '\0')
    {
        // no argument found
        r.found = 0;
        return r;
    }
    size_t start_index = cur;
    r.start = &command[cur];
    while ((command[cur] != endChar) && (command[cur] != '\0'))
    {
        if (endChar == '"' && command[cur] != ' ')
        {
            insideQuoteEmpty = 0;
        }
        cur++;
    }
    r.len = cur - start_index;
    if (endChar == '"' && r.len == 0)
    {
        r.found = 0;
        return r;
    }
    if (endChar == '"' && insideQuoteEmpty)
    {
        r.found = 0;
        return r;
    }
    r.found = 1;
    // at this point command[cur] points at the delim char, either space or quote
    if (command[cur] != '\0')
    {
        cur++;
    }
    r.nextStart = &command[cur];
    return r;
}

void test_findNextArg()
{
    argument r = findNextArg("hello world > \"blah blah\" haha");
    printf("found=%d, start=%s, len=%d\n", r.found, r.start, r.len);
    argument r2 = findNextArg(r.nextStart);
    printf("found=%d, start=%s, len=%d\n", r2.found, r2.start, r2.len);
    argument r3 = findNextArg(r2.nextStart);
    printf("found=%d, start=%s, len=%d\n", r3.found, r3.start, r3.len);
    argument r4 = findNextArg(r3.nextStart);
    printf("found=%d, start=%s, len=%d\n", r4.found, r4.start, r4.len);
    argument r5 = findNextArg(r4.nextStart);
    printf("found=%d, start=%s, len=%d\n", r5.found, r5.start, r5.len);
    argument r6 = findNextArg(r5.nextStart);
    printf("found=%d", r6.found);
}

void test_backend()
{
    shell_mkdir("dir1");
    shell_mkdir("dir1/dir2");
    //    shell_cp("README", "dir1/dir2/COPY");
    shell_cd("dir1/dir2");
    //    shell_echo("hello", "COPY");
    int fd = open("COPY", O_RDWR | O_CREATE);
    printf("%d\n", fd);
    //    close(fd);
    shell_pwd();
}

void test_cwd()
{
    char buffer[300];
    int pathLen = getcwd();
    if (pathLen <= 0)
    {
        printf("cwd failed\n");
    }
    printf("user side cwd succeeded, path=%s\n", buffer);
    shell_mkdir("folder1");
    shell_cd("folder1");
    shell_mkdir("folder2");
    shell_cd("folder2");
    pathLen = getcwd();
    if (pathLen <= 0)
    {
        printf("cwd failed\n");
    }
    printf("user side cwd succeeded, path=%s\n", buffer);
}
void test_nested()
{
    for (int i = 0; i < 100; i++)
    {
        shell_mkdir("test");
        shell_cd("test");
    }
    shell_pwd();
}

#define ARG_CMP(parsed_name, target_name) strncmp((parsed_name).start, target_name, MAX((parsed_name).len, strlen(target_name)))
#define COPY_ARG(arg, buffer)                    \
    {                                            \
        strncpy(buffer, (arg).start, (arg).len); \
        (buffer)[(arg).len] = '\0';              \
    }

#define CHECK_ARG(arg, message)      \
    {                                \
        if (!(arg).found)            \
        {                            \
            printf("%s\n", message); \
            continue;                \
        }                            \
    }

#define CHECK_ARG_LIMIT(arg, message)                     \
    {                                                     \
        argument next_arg = findNextArg((arg).nextStart); \
        if ((next_arg).found)                             \
        {                                                 \
            printf("%s\n", message);                      \
            continue;                                     \
        }                                                 \
    }
void shell_loop()
{
    char command[1024];
    while (sys_readline("$ ", command) == 0)
    {
        argument name = findNextArg(command);
        if (!name.found)
        {
            continue;
        }
        if (ARG_CMP(name, "ls") == 0)
        {
            argument arg1 = findNextArg(name.nextStart);
            if (!arg1.found)
            {
                shell_ls(".");
            }
            else
            {
                char arg1_copy[arg1.len + 1];
                COPY_ARG(arg1, arg1_copy);

                CHECK_ARG_LIMIT(arg1, "usage: ls OR ls dirname");

                shell_ls(arg1_copy);
            }
        }
        else if (ARG_CMP(name, "pwd") == 0)
        {
            CHECK_ARG_LIMIT(name, "usage: pwd");
            shell_pwd();
        }
        else if (ARG_CMP(name, "cd") == 0)
        {
            argument arg1 = findNextArg(name.nextStart);
            if (!arg1.found)
            {
                shell_cd("/");
            }
            else
            {
                CHECK_ARG(arg1, "usage: cd dirname");
                char arg1_copy[arg1.len + 1];
                COPY_ARG(arg1, arg1_copy);

                CHECK_ARG_LIMIT(arg1, "usage: cd dirname");

                shell_cd(arg1_copy);
            }
        }
        else if (ARG_CMP(name, "cp") == 0)
        {
            argument arg1 = findNextArg(name.nextStart);
            CHECK_ARG(arg1, "usage: cp src dest OR cp -r srcDir destDir");
            // Recursive cp
            if (ARG_CMP(arg1, "-r") == 0)
            {
                // Get the source
                argument source = findNextArg(arg1.nextStart);
                CHECK_ARG(source, "usage: cp src dest OR cp -r srcDir destDir");
                char source_copy[source.len + 1];
                COPY_ARG(source, source_copy);

                // Get the destination
                argument dest = findNextArg(source.nextStart);
                CHECK_ARG(dest, "usage: cp src dest OR cp -r srcDir destDir");
                char dest_copy[dest.len + 1];
                COPY_ARG(dest, dest_copy);

                CHECK_ARG_LIMIT(dest, "usage: cp src dest OR cp -r srcDir destDir");
                shell_cp_r(source_copy, dest_copy);
            }
            else
            {
                char source_copy[arg1.len + 1];
                COPY_ARG(arg1, source_copy);

                argument dest = findNextArg(arg1.nextStart);
                CHECK_ARG(dest, "usage: cp src dest OR cp -r srcDir destDir");
                char dest_copy[dest.len + 1];
                COPY_ARG(dest, dest_copy);
                shell_cp(source_copy, dest_copy);
            }
        }
        else if (ARG_CMP(name, "mv") == 0)
        {
            argument source = findNextArg(name.nextStart);
            CHECK_ARG(source, "usage: mv src dest");
            char source_copy[source.len + 1];
            COPY_ARG(source, source_copy);

            argument dest = findNextArg(source.nextStart);
            CHECK_ARG(dest, "usage: mv src dest");
            char dest_copy[dest.len + 1];
            COPY_ARG(dest, dest_copy);
            CHECK_ARG_LIMIT(dest, "usage: mv src dest");
            shell_mv(source_copy, dest_copy);
        }
        else if (ARG_CMP(name, "rm") == 0)
        {
            argument arg1 = findNextArg(name.nextStart);
            CHECK_ARG(arg1, "usage: rm filename OR rm -r dirname");

            if (ARG_CMP(arg1, "-r") == 0)
            {
                argument arg2 = findNextArg(arg1.nextStart);
                CHECK_ARG(arg2, "usage: rm filename OR rm -r dirname");

                char arg2_copy[arg2.len + 1];
                COPY_ARG(arg2, arg2_copy);
                CHECK_ARG_LIMIT(arg2, "usage: rm filename OR rm -r dirname");
                shell_rm_r(arg2_copy);
            }
            else
            {
                char arg1_copy[arg1.len + 1];
                COPY_ARG(arg1, arg1_copy);
                CHECK_ARG_LIMIT(arg1, "usage: rm filename OR rm -r dirname");
                shell_rm(arg1_copy);
            }
        }
        else if (ARG_CMP(name, "mkdir") == 0)
        {
            argument arg1 = findNextArg(name.nextStart);
            CHECK_ARG(arg1, "usage: mkdir dirname");
            char arg1_copy[arg1.len + 1];
            COPY_ARG(arg1, arg1_copy);
            CHECK_ARG_LIMIT(arg1, "usage: mkdir dirname");
            shell_mkdir(arg1_copy);
        }
        else if (ARG_CMP(name, "cat") == 0)
        {
            argument file = findNextArg(name.nextStart);
            CHECK_ARG(file, "usage: cat filename")
            char file_copy[file.len + 1];
            COPY_ARG(file, file_copy);

            CHECK_ARG_LIMIT(file, "usage: cat filename")
            shell_cat(file_copy);
        }
        else if (ARG_CMP(name, "echo") == 0)
        {
            argument text = findNextArg(name.nextStart);
            CHECK_ARG(text, "usage: echo \"text\" > filename OR echo \"text\" >> filename");
            char text_copy[text.len + 1];
            COPY_ARG(text, text_copy);

            argument arg2 = findNextArg(text.nextStart);
            CHECK_ARG(arg2, "usage: echo \"text\" > filename OR echo \"text\" >> filename");

            argument fileName = findNextArg(arg2.nextStart);
            CHECK_ARG(fileName, "usage: echo \"text\" > filename OR echo \"text\" >> filename");

            char fileName_copy[fileName.len + 1];
            COPY_ARG(fileName, fileName_copy);

            CHECK_ARG_LIMIT(fileName, "usage: echo \"text\" > filename OR echo \"text\" >> filename");
            if (ARG_CMP(arg2, ">") == 0)
            {
                shell_echo(text_copy, fileName_copy);
            }
            else if (ARG_CMP(arg2, ">>") == 0)
            {
                shell_echo_append(text_copy, fileName_copy);
            }
            else
            {
                printf("usage: echo \"text\" > filename OR echo \"text\" >> filename");
            }
        }
        else if (ARG_CMP(name, "touch") == 0)
        {
            argument file_name = findNextArg(name.nextStart);
            CHECK_ARG(file_name, "usage: touch filename");
            char file_name_copy[file_name.len + 1];
            COPY_ARG(file_name, file_name_copy);

            CHECK_ARG_LIMIT(file_name, "usage: touch filename");
            shell_touch(file_name_copy);
        }
        else
        {
            char name_copy[name.len + 1];
            COPY_ARG(name, name_copy);
            printf("command not found: %s\n", name_copy);
        }
    }
}
int main(int argc, char *argv[])
{
    //    test_backend();
    //    test_nested();
    shell_loop();
    return 0;
}