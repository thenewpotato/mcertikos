#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <x86.h>
#include <file.h>
#include <gcc.h>

#define exit(...) return __VA_ARGS__
#define T_DIR  1  // Directory
#define T_FILE 2  // File
#define T_DEV  3  // Device
#define IO_CHUNK_SIZE 512   // Reading and writing in chunks of 512 bytes

/*
"" or "    " => all fields NULL
"cp" or "     cp" => name=cp, all other fields NULL
"cp blah"
*/
int parseOld(char *line, char **args) {
    int argi = 0;
    size_t linei = 0;
    while (line[linei] != '\0') {
        if (argi >= 6) {
            return -1;
        }

        size_t argstart = linei;
        if (line[argstart] == '"') {
            while (line[linei] != '"') {
                linei++;
            }
        }
        while (line[linei] != ' ' && line[linei] != '\0') {
            printf("line");
            linei++;
        }

        while (line[linei] != ' ' && line[linei] != '\0') {
            printf("line");
            linei++;
        }

        size_t argend = linei;
        size_t arglen = argend - argstart;
        printf("parse %d %d\n", argstart, argend);
        for (int j = argstart; j < arglen; j++) {
            printf("%c", line[j]);
        }
        // printf("\n");
        if (arglen == 0 || arglen > 128) {
            return -1;
        } else {
            strncpy(args[argi], &line[argstart], arglen);
            args[argi][arglen] = '\0';
        }

        argi++;
        linei++;
    }
    return argi + 1;
}

// Definition of dirent copied over from kernel, needed by ls to iterate over dirents
#define DIRSIZ 14
struct dirent {
    uint16_t inum;
    char name[DIRSIZ];
};

void shell_ls(char *relativePath) {
    int dirfd;
    size_t nDirents;
    struct file_stat stat;

    // TODO: error handling
    dirfd = open(relativePath, O_RDONLY);
    fstat(dirfd, &stat);
    if (stat.size % sizeof(struct dirent) != 0)
        printf("dir file corrupted\n");
    nDirents = stat.size / sizeof(struct dirent);
    for (size_t i = 0; i < nDirents; i++) {
        struct dirent cur_dirent;

        read(dirfd, (char *) &cur_dirent, sizeof(struct dirent));
        if (cur_dirent.inum == 0)   // inum 0 indicates empty entry
            break;
        printf("%s\t", cur_dirent.name);
    }
    printf("\n");
    close(dirfd);
}

void shell_mkdir(char *name) {
    int mkdirStatus;

    mkdirStatus = mkdir(name);
    if (mkdirStatus != 0)
        printf("mkdir: no such file or directory: %s\n", name);
}

void shell_cd(char *relativePath) {
    if (chdir(relativePath) != 0) {
        printf("cd: no such file or directory: %s\n", relativePath);
        return;
    }
}

void shell_touch(char *relativePath) {
    int fd = open(relativePath, O_CREATE);
    if (fd >= 0) {
        close(fd);
    } else {
        printf("touch: unable to create file: %s\n", relativePath);
    }
}

void shell_cat(char *relativePath) {
    struct file_stat stat;

    int fd = open(relativePath, O_RDONLY);
    if (fd < 0) {
        printf("cat: no such file: %s\n", relativePath);
        return;
    }
    fstat(fd, &stat);
    if(stat.type != T_FILE){
        printf("cat: %s: is a directory\n", relativePath);
    }


    char content[stat.size + 1];
    printf("cat size %d\n", stat.size);
    read(fd, content, stat.size);
    content[stat.size] = '\0';
    printf("%s", content);
    close(fd);
}

void shell_mv(char *src, char *dst) {
    // NOTE: current mv syntax is NOT regular shell syntax, second argument must be file path
    int linkStatus = link(src, dst);
    int unlinkStatus = unlink(src);
    if (linkStatus != 0 || unlinkStatus != 0) {
        printf("mv failed\n");
    }
}

/*
 * If relativePath specifies a file, unlink the file
 * If relativePath specifies a directory, recursively unlink its contents and the directory
 * Returns 0 on success, -1 on failure
 */
int remove(char *relativePath) {
    int fd_root = open(relativePath, O_RDONLY);

    ASSERT(fd_root >= 0);

    struct file_stat fstat_root;
    int fstat_status = fstat(fd_root, &fstat_root);

    ASSERT(fstat_status == 0);
    ASSERT(fstat_root.type == T_DIR || fstat_root.type == T_FILE);

    if (fstat_root.type == T_FILE) {
        close(fd_root);
        int unlinkStatus = unlink(relativePath);

        ASSERT(unlinkStatus == 0);
        printf("remove: removed file %s\n", relativePath);
    } else {
        int chdir_status = chdir(relativePath);
        printf("remove: removing dir %s\n", relativePath);

        ASSERT(chdir_status == 0);
        ASSERT(fstat_root.size % sizeof(struct dirent) == 0);

        size_t nDirents = fstat_root.size / sizeof(struct dirent);
        for (size_t i = 0; i < nDirents; i++) {
            struct dirent cur_dirent;

            int read_status = read(fd_root, (char *) &cur_dirent, sizeof(struct dirent));
            ASSERT(read_status > 0);
            if (cur_dirent.inum == 0)   // inum 0 indicates empty entry
                break;
            if (strcmp(cur_dirent.name, ".") == 0 || strcmp(cur_dirent.name, "..") == 0)
                continue;
            printf("remove: recursively removing %s\n", cur_dirent.name);
            int recursive_remove_status = remove(cur_dirent.name);
            ASSERT(recursive_remove_status == 0);
        }

        chdir_status = chdir("..");

        ASSERT(chdir_status == 0);

        close(fd_root);
        int unlinkStatus = unlink(relativePath);

        ASSERT(unlinkStatus == 0);
        printf("remove: removed dir %s\n", relativePath);
    }
    return 0;
}

void shell_rm(char *relativePath) {
    int fd = open(relativePath, O_RDONLY);
    if (fd < 0) {
        printf("rm: no such file or directory: %s\n", relativePath);
        return;
    }
    struct file_stat stat;
    int fstat_status = fstat(fd, &stat);
    if (fstat_status != 0) {
        printf("rm: no such file or directory: %s\n", relativePath);
        return;
    }
    if (stat.type != T_FILE) {
        printf("rm: not a file: %s\n", relativePath);
        return;
    }
    int close_status = close(fd);
    ASSERT(close_status == 0);
    int remove_status = remove(relativePath);
    ASSERT(remove_status == 0);
}

void shell_rm_r(char *relativePath) {
    int fd = open(relativePath, O_RDONLY);
    if (fd < 0) {
        printf("rm: no such file or directory: %s\n", relativePath);
        return;
    }
    struct file_stat stat;
    int fstat_status = fstat(fd, &stat);
    if (fstat_status != 0) {
        printf("rm: no such file or directory: %s\n", relativePath);
        return;
    }
    if (stat.type != T_DIR) {
        printf("rm: not a directory: %s\n", relativePath);
        return;
    }
    int close_status = close(fd);
    ASSERT(close_status == 0);
    int remove_status = remove(relativePath);
    ASSERT(remove_status == 0);
}

/*
 *
 */
int copy(char *srcPath, char *destPath) {
    int fd_src = open(srcPath, O_RDONLY);
    int fd_dest = open(destPath, O_RDONLY);
    ASSERT(fd_src >= 0);
    ASSERT(fd_dest < 0);
    struct file_stat fstat_src;
    ASSERT(fstat(fd_src, &fstat_src) == 0);
    ASSERT(fstat_src.type == T_FILE || fstat_src.type == T_DIR);
    if (fstat_src.type == T_FILE) {
        ASSERT(close(fd_dest) == 0);
        fd_dest = open(destPath, O_RDWR | O_CREATE);
        ASSERT(fd_dest >= 0);
        char buffer[IO_CHUNK_SIZE];
        while (read(fd_src, buffer, IO_CHUNK_SIZE) > 0) {
            ASSERT(write(fd_dest, buffer, IO_CHUNK_SIZE) > 0);
        }
    } else {

    }
}

void shell_cp(char *src, char *dst) {
    int fdSrc = open(src, O_RDONLY);
    int fdDest = open(dst, O_RDWR | O_CREATE);
    if (fdSrc < 0) {
        printf("cp: no such file: %s\n", src);
        return;
    }
    if (fdDest < 0) {
        printf("cp: no such file: %s\n", dst);
        return;
    }
    struct file_stat statSrc;
    fstat(fdSrc, &statSrc);
    char contentSrc[statSrc.size];
    int readStatus = read(fdSrc, contentSrc, statSrc.size);
    int writeStatus = write(fdDest, contentSrc, statSrc.size);
    if (readStatus < 0 || writeStatus < 0) {
        printf("cp: read/write failed\n");
    }
    close(fdSrc);
    close(fdDest);
}

void shell_echo(char *text, char *relativePath) {
    int fd = open(relativePath, O_RDWR | O_CREATE);
    if (fd < 0) {
        printf("echo: no such file: %s\n", relativePath);
        return;
    }
    int writeStatus = write(fd, text, strlen(text));
    if (writeStatus < 0) {
        printf("echo: write failed\n");
    }
    close(fd);
}

void shell_echo_append(char *text, char *relativePath) {
    int fd = open(relativePath, O_RDWR | O_CREATE);
    if (fd < 0) {
        printf("echo: no such file: %s\n", relativePath);
        return;
    }
    char unused_buffer[IO_CHUNK_SIZE];
    while (read(fd, unused_buffer, IO_CHUNK_SIZE) > 0) {}
    int writeStatus = write(fd, text, strlen(text));
    if (writeStatus < 0) {
        printf("echo: write failed\n");
    }
    close(fd);
}

void shell_pwd() {
//    printf("not implemented yet\n");
    if(getcwd() == -1){
        printf("pwd failed\n");
    }
//    return;
//    char buffer[300];
//    int pathLen = getcwd(buffer);
//    if (pathLen <= 0) {
//        printf("cwd failed\n");
//    }

}

typedef struct {
    int found;
    const char *start;
    const char *nextStart;
    size_t len;
} argument;

argument findNextArg(const char *command) {
    printf("find %s\n", command);
    argument r;
    size_t cur = 0;
    char endChar = ' ';
    while (command[cur] == ' ') {
        cur++;
    }
    if (command[cur] == '"') {
        cur++;
        endChar = '"';
    } else if (command[cur] == '\0') {
        // no argument found
        r.found = 0;
        return r;
    }
    size_t start_index = cur;
    r.start = &command[cur];
    while ((command[cur] != endChar) && (command[cur] != '\0')) {
        cur++;
    }
    r.len = cur - start_index;
    r.found = 1;
    // at this point command[cur] points at the delim char, either space or quote
    if (command[cur] != '\0') {
        cur++;
    }
    r.nextStart = &command[cur];
    return r;
}

void test_findNextArg() {
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

void test_backend() {
    shell_mkdir("blah/blah");
    shell_mkdir("testDir");
    shell_cd("testDir");
    shell_mkdir("nested");
    shell_touch("test.txt");
    shell_touch("test2.txt");
    shell_echo("hello world!\n", "test2.txt");
    shell_cat("test2.txt");
    shell_cp("test2.txt", "test3.txt");
    shell_cat("test3.txt");
    shell_echo("goodbye world!\n", "test2.txt");
    shell_cat("test2.txt");
    shell_cat("test3.txt");
    shell_ls(".");
    shell_mv("test2.txt", "../test2.txt");
    shell_ls(".");
    shell_rm("test.txt");
    shell_ls(".");
    shell_rm("test.txt");
    shell_ls("..");
}

void test_cwd() {
    char buffer[300];
    int pathLen = getcwd();
    if (pathLen <= 0) {
        printf("cwd failed\n");
    }
    printf("user side cwd succeeded, path=%s\n", buffer);
    shell_mkdir("folder1");
    shell_cd("folder1");
    shell_mkdir("folder2");
    shell_cd("folder2");
    pathLen = getcwd();
    if (pathLen <= 0) {
        printf("cwd failed\n");
    }
    printf("user side cwd succeeded, path=%s\n", buffer);
}

#define ARG_CMP(parsed_name, target_name)  strncmp((parsed_name).start, target_name, MAX((parsed_name).len, strlen(target_name)))
#define COPY_ARG(arg, buffer) { \
    strncpy(buffer, (arg).start, (arg).len); \
    (buffer)[(arg).len] = '\0'; \
}
#define CHECK_ARG(arg, message) { \
    if (!(arg).found) {             \
        printf("%s\n", message);  \
        continue;                 \
    } \
}


int main(int argc, char *argv[]) {
//    test_cwd();

    char command[1024];

    while (sys_readline("$ ", command) == 0) {
        argument name = findNextArg(command);
        if (!name.found) {
            continue;
        }
        printf("received command: %s (%d)\n", name.start, name.len);
        if (ARG_CMP(name, "ls") == 0) {
            argument arg1 = findNextArg(name.nextStart);
            CHECK_ARG(arg1, "ls usage...");
            char arg1_copy[arg1.len + 1];
            COPY_ARG(arg1, arg1_copy);
            shell_ls(arg1_copy);


            argument check = findNextArg(arg1.nextStart);
            if(check.found){
                printf("cp usage ...\n");
                continue;
            }
        } else if (ARG_CMP(name, "pwd") == 0) {
            shell_pwd();

            argument check = findNextArg(name.nextStart);
            if(check.found){
                printf("cp usage ...\n");
                continue;
            }
        } else if (ARG_CMP(name, "cd") == 0) {
            argument arg1 = findNextArg(name.nextStart);
            CHECK_ARG(arg1, "cd usage...");
            char arg1_copy[arg1.len + 1];
            COPY_ARG(arg1, arg1_copy);
            shell_cd(arg1_copy);

            argument check = findNextArg(arg1.nextStart);
            if(check.found){
                printf("cp usage ...\n");
                continue;
            }
        }else if (ARG_CMP(name, "cp") == 0) {
            argument arg1 = findNextArg(name.nextStart);
            CHECK_ARG(arg1, "cp usage...");
            // Recursive cp
            if (ARG_CMP(arg1, "-r") == 0) {
                // Get the source
                argument source = findNextArg(arg1.nextStart);
                CHECK_ARG(source, "cp usage...");
                char source_copy[source.len + 1];
                COPY_ARG(source, source_copy);

                // Get the destination
                argument dest = findNextArg(source.nextStart);
                CHECK_ARG(dest, "cp usage...");
                char dest_copy[dest.len + 1];
                COPY_ARG(dest, dest_copy);

                // TODO: shell_cp -r


                argument check = findNextArg(dest.nextStart);
                if(check.found){
                    printf("cp usage ...\n");
                    continue;
                }

            } else {
                char source_copy[arg1.len + 1];
                COPY_ARG(arg1, source_copy);

                argument dest = findNextArg(arg1.nextStart);
                CHECK_ARG(dest, "cp usage...");
                char dest_copy[dest.len + 1];
                COPY_ARG(dest, dest_copy);
                shell_cp(source_copy, dest_copy);
                argument check = findNextArg(dest.nextStart);
                if(check.found){
                    printf("cp usage ...\n");
                    continue;
                }
            }

        }
        else if(ARG_CMP(name, "mv") == 0){
            argument source = findNextArg(name.nextStart);
            CHECK_ARG(source, "mv usage ...");
            char source_copy[source.len + 1];
            COPY_ARG(source, source_copy);

            argument dest = findNextArg(source.nextStart);
            CHECK_ARG(dest, "mv usage ...");
            char dest_copy[dest.len + 1];
            COPY_ARG(dest, dest_copy);
            shell_mv(source_copy, dest_copy);


            argument check = findNextArg(dest.nextStart);
            if(check.found){
                printf("mv usage ...\n");
                continue;
            }
        }
        else if (ARG_CMP(name, "rm") == 0) {
            argument arg1 = findNextArg(name.nextStart);
            if (ARG_CMP(arg1, "-r") == 0) {
                argument arg2 = findNextArg(arg1.nextStart);
                char arg2_copy[arg2.len + 1];
                COPY_ARG(arg2, arg2_copy);
                shell_rm_r(arg2_copy);
                argument check = findNextArg(arg2.nextStart);
                if(check.found){
                    printf("rm usage ...\n");
                    continue;
                }
            } else {
                char arg1_copy[arg1.len + 1];
                COPY_ARG(arg1, arg1_copy);
                shell_rm(arg1_copy);
                argument check = findNextArg(arg1.nextStart);
                if(check.found){
                    printf("cat usage ...\n");
                    continue;
                }
            }
        }
        else if (ARG_CMP(name, "mkdir") == 0) {
            argument arg1 = findNextArg(name.nextStart);
            CHECK_ARG(arg1, "mkdir usage...");
            char arg1_copy[arg1.len + 1];
            COPY_ARG(arg1, arg1_copy);

            shell_mkdir(arg1_copy);
            argument check = findNextArg(arg1.nextStart);
            if(check.found){
                printf("mkdir usage ...\n");
                continue;
            }
        }
        else if(ARG_CMP(name, "cat") == 0){
            argument file = findNextArg(name.nextStart);
            CHECK_ARG(file, "cat usage ...")
            char file_copy[file.len + 1];
            COPY_ARG(file, file_copy);
            shell_cat(file_copy);

            argument check = findNextArg(file.nextStart);
            if(check.found){
                printf("cat usage ...\n");
                continue;
            }
        }
        else if (ARG_CMP(name, "echo") == 0) {
            argument text = findNextArg(name.nextStart);
            char text_copy[text.len + 1];
            COPY_ARG(text, text_copy);
            argument arg2 = findNextArg(text.nextStart);
            argument fileName = findNextArg(arg2.nextStart);
            char fileName_copy[fileName.len + 1];
            COPY_ARG(fileName, fileName_copy);
            if (ARG_CMP(arg2, ">") == 0) {
                shell_echo(text_copy, fileName_copy);
            } else if (ARG_CMP(arg2, ">>") == 0) {
                shell_echo_append(text_copy, fileName_copy);
            }
            argument check = findNextArg(fileName.nextStart);
            if(check.found){
                printf("cat usage ...\n");
                continue;
            }
        }
//        else if(ARG_CMP)
    }
    return 0;
}