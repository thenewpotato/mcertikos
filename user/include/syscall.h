#ifndef _USER_SYSCALL_H_
#define _USER_SYSCALL_H_

#include <lib/syscall.h>

#include <debug.h>
#include <gcc.h>
#include <proc.h>
#include <types.h>
#include <x86.h>
#include <file.h>

static gcc_inline void sys_puts(const char *s, size_t len)
{
  asm volatile("int %0" ::"i"(T_SYSCALL),
               "a"(SYS_puts),
               "b"(s),
               "c"(len)
               : "cc", "memory");
}

static gcc_inline pid_t sys_spawn(unsigned int elf_id, unsigned int quota)
{
  int errno;
  pid_t pid;

  asm volatile("int %2"
               : "=a"(errno), "=b"(pid)
               : "i"(T_SYSCALL),
                 "a"(SYS_spawn),
                 "b"(elf_id),
                 "c"(quota)
               : "cc", "memory");

  return errno ? -1 : pid;
}

static gcc_inline void sys_yield(void)
{
  asm volatile("int %0" ::"i"(T_SYSCALL),
               "a"(SYS_yield)
               : "cc", "memory");
}

static gcc_inline int sys_read(int fd, char *buf, size_t n)
{
  int errno;
  size_t ret;

  asm volatile("int %2"
               : "=a"(errno), "=b"(ret)
               : "i"(T_SYSCALL),
                 "a"(SYS_read),
                 "b"(fd),
                 "c"(buf),
                 "d"(n)
               : "cc", "memory");

  return errno ? -1 : ret;
}

static gcc_inline int sys_write(int fd, char *p, int n)
{
  int errno;
  size_t ret;

  asm volatile("int %2"
               : "=a"(errno), "=b"(ret)
               : "i"(T_SYSCALL),
                 "a"(SYS_write),
                 "b"(fd),
                 "c"(p),
                 "d"(n)
               : "cc", "memory");

  return errno ? -1 : ret;
}

static gcc_inline int sys_close(int fd)
{
  int errno;
  int ret;

  asm volatile("int %2"
               : "=a"(errno), "=b"(ret)
               : "i"(T_SYSCALL),
                 "a"(SYS_close),
                 "b"(fd)
               : "cc", "memory");

  return errno ? -1 : 0;
}

static gcc_inline int sys_fstat(int fd, struct file_stat *st)
{
  int errno;
  int ret;

  asm volatile("int %2"
               : "=a"(errno), "=b"(ret)
               : "i"(T_SYSCALL),
                 "a"(SYS_stat),
                 "b"(fd),
                 "c"(st)
               : "cc", "memory");

  return errno ? -1 : 0;
}

static gcc_inline int sys_link(char *old, char *new, unsigned int old_length, unsigned int new_length)
{
  int errno, ret;

  asm volatile("int %4"
               : "=a"(errno), "=b"(ret)
               : "i"(T_SYSCALL),
                 "a"(SYS_link),
                 "b"(old),
                 "c"(new),
                 "d"(old_length),
                 "e"(new_length)
               : "cc", "memory");

  return errno ? -1 : 0;
}

static gcc_inline int sys_unlink(char *path, unsigned int length)
{
  int errno, ret;

  asm volatile("int %2"
               : "=a"(errno), "=b"(ret)
               : "i"(T_SYSCALL),
                 "a"(SYS_unlink),
                 "b"(path),
                 "c"(length)
               : "cc", "memory");

  return errno ? -1 : 0;
}

static gcc_inline int sys_open(char *path, int omode, unsigned int path_length)
{
  int errno;
  int fd;

  asm volatile("int %2"
               : "=a"(errno), "=b"(fd)
               : "i"(T_SYSCALL),
                 "a"(SYS_open),
                 "b"(path),
                 "c"(omode),
                 "d"(path_length)
               : "cc", "memory");

  return errno ? -1 : fd;
}

static gcc_inline int sys_mkdir(char *path, unsigned int path_length)
{
  int errno, ret;

  asm volatile("int %2"
               : "=a"(errno), "=b"(ret)
               : "i"(T_SYSCALL),
                 "a"(SYS_mkdir),
                 "b"(path),
                 "c"(path_length)
               : "cc", "memory");

  return errno ? -1 : 0;
}

static gcc_inline int sys_chdir(char *path, unsigned int path_length)
{
  int errno, ret;

  asm volatile("int %2"
               : "=a"(errno), "=b"(ret)
               : "i"(T_SYSCALL),
                 "a"(SYS_chdir),
                 "b"(path),
                 "c"(path_length)
               : "cc", "memory");

  return errno ? -1 : 0;
}

#endif /* !_USER_SYSCALL_H_ */
