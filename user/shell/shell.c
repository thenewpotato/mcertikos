#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <x86.h>
#include <file.h>
#include <gcc.h>

#define exit(...) return __VA_ARGS__

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

void shell_rm(char *relativePath) {
    int unlinkStatus = unlink(relativePath);
    if (unlinkStatus != 0) {
        printf("rm: no such file or directory: %s\n", relativePath);
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

void shell_echo(char *text, char *relativePath, int overwrite) {
    if (overwrite) {
        int fd = open(relativePath, O_RDWR);
        if (fd < 0) {
            printf("echo: no such file: %s\n", relativePath);
            return;
        }
        int writeStatus = write(fd, text, strlen(text));
        if (writeStatus < 0) {
            printf("echo: write failed\n");
        }
        close(fd);
    } else {    // append mode
        printf("not implemented yet\n");
    }
}

void shell_pwd() {
    printf("not implemented yet\n");
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
    shell_echo("hello world!\n", "test2.txt", 1);
    shell_cat("test2.txt");
    shell_cp("test2.txt", "test3.txt");
    shell_cat("test3.txt");
    shell_echo("goodbye world!\n", "test2.txt", 1);
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
    shell_mkdir("folder1");
    shell_cd("folder1");
    shell_mkdir("folder2");
    shell_cd("folder2");
    char buffer[300];
    int pathLen = getcwd(buffer);
    if (pathLen <= 0) {
        printf("cwd failed\n");
    }
    printf("user side cwd succeeded, path=%s\n", buffer);
}

#define CMD_NAME_CMP(parsed_name, target_name)  strncmp(parsed_name.start, target_name, MAX(parsed_name.len, strlen(target_name)))
#define COPY_ARG(arg, buffer) { \
    strncpy(buffer, arg.start, arg.len); \
    buffer[arg.len] = '\0'; \
}
#define CHECK_ARG(arg, message) { \
    if (!arg.found) {             \
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
        if (CMD_NAME_CMP(name, "ls") == 0) {
            argument arg1 = findNextArg(name.nextStart);
            CHECK_ARG(arg1, "ls usage...");
            char arg1_copy[arg1.len + 1];
            COPY_ARG(arg1, arg1_copy);

            shell_ls(arg1_copy);
        } else if (CMD_NAME_CMP(name, "cd") == 0) {
            argument arg1 = findNextArg(name.nextStart);
            CHECK_ARG(arg1, "cd usage...");
            char arg1_copy[arg1.len + 1];
            COPY_ARG(arg1, arg1_copy);

            shell_cd(arg1_copy);
        } else if (CMD_NAME_CMP(name, "mkdir") == 0) {
            argument arg1 = findNextArg(name.nextStart);
            CHECK_ARG(arg1, "mkdir usage...");
            char arg1_copy[arg1.len + 1];
            COPY_ARG(arg1, arg1_copy);

            shell_mkdir(arg1_copy);
        }
    }
    return 0;
}