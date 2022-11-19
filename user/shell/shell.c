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
int parse(char *line, char **args)
{
    /*
    normalarg(start)
    quotearg(start)
    none
    */
    int argi = 0;
    size_t linei = 0;
    while (line[linei] != '\0') {
        if (argi >= 6) {
            return -1;
        }

        size_t argstart = linei;
        if (line[argstart] == '"') {
            while(line[linei] != '"') {
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
        for(int j = argstart;  j < arglen; j++){
            printf("%c", line[j]);
        }          
        // printf("\n");
        if (arglen == 0 || arglen > 128) {
            return -1;
        }
        else{
            strncpy(args[argi], &line[argstart], arglen);
            args[argi][arglen] = '\0';
        }
        
        argi++;
        linei++;
    }
    return argi + 1;
}

int ls(char* path) {
    return 0;
}

int pwd() {
    return 0;
}

int main(int argc, char *argv[])
{
    // int fd = open("/", O_RDONLY);
    // printf("fd %d\n", fd);
    // struct file_stat st;
    // if (fstat(fd, &st) != 0)
    // {
    //     printf("error: file_stat failed\n");
    // }
    // printf("file_stat: type=%d, dev=%d, ino=%d, nlink=%d, size=%d\n", st.type, st.dev, st.ino, st.nlink, st.size);
    // return 0;
    char buffer[1024];

    while (sys_readline("$ ", buffer) == 0)
    {
        char args[6][129];
        int nargs = parse("hello world", args);
        // for(unsigned int i = 0; i < nargs; i++){
        //     printf("%s", args[i]);
        // }
        return 0;
        /*
        ls
        cat hello.txt
        cp src dest
        cp -r src dest => [cp, -r, src, dest]

        echo "some content" > file => [echo, some content],
        echo "some content" >> file
        */
        if (strcmp(buffer, "ls") == 0)
        {
        }
        else if (strcmp(buffer, "pwd") == 0)
        {
        }
        else if (strcmp(buffer, "cd") == 0)
        {
        }
        else if (strcmp(buffer, "cp") == 0)
        {
        }
        else if (strcmp(buffer, "mv") == 0)
        {
        }
        else if (strcmp(buffer, "rm") == 0)
        {
        }
        else if (strcmp(buffer, "mkdir") == 0)
        {
        }
        else if (strcmp(buffer, "cat") == 0)
        {
        }
        else if (strcmp(buffer, "touch") == 0)
        {
        }
        else if (strcmp(buffer, "redirect") == 0)
        {
        }

        // File redirect
        else if (strcmp(buffer, "") == 0)
        {
        }
        // File reidrect append
        else if (strcmp(buffer, "") == 0)
        {
        }
        else
        {
            printf("Invalid Shell Command\n");
        }
    }

    printf("readline failed\n");
    return 1;
}