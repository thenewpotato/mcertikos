Compile: make / make all
Run tests: make clean && make TEST=1
Run in qemu: make qemu / make qemu-nox
Debug with gdb: make qemu-gdb / make qemu-nox-gdb
(in another terminal) gdb

To use your solutions from lab 1: git merge lab1
To use sample lab 1 solutions: copy files in samples/ to appropriate directories

List here the following info:

1. who you have worked with
2. whether you coded this assignment together, and if not, who worked on which part
3. brief description of what you have implemented
4. and anything else you would like us to know

# Collaborators

- Tiger Wang (jw2723)
- Jackie Dong (jd2598)

# Running
Currently, `make qemu-nox` runs the shell program that we have written in `user/shell/shell.c`. In order to run `fstest`, comment out line 29 of `idle.c` and uncomment line 28. 

# Part 1: Scheduler Support: Sleep and Wakeup

In this part, we implemented more scheduler support in mcertikos. Specifically, we implemented thread_sleep and thread_wakeup which allow threads to sleep until certain resources are available for them to use. This is important specifically for disk writes where, we need to enable our threads to sleep until the disk write carries out. When the write is finished, the hard disk issues a special device interrupt signaling the threads that were waiting for the write to finish to wake up. To sleep a process on a certain channel, we set the process `channel` to the appropriate pointer, set its state to sleeping, and enqueue it to queue 0, which we use as the waiting queue. In `thread_wakeup`, we wake up all threads sleeping with the specified channel. To do so, we loop through the sleeping queue, dequeueing each thread that has the specified channel, setting their state to `TSTATE_READY`, and enqueueing them to the ready queue.

# Part 2: Disk Driver

In this part, we just had to read the documentation about the xv6 file system.

# Part 3: File System Support

In this part we first implemented the functions `dir_lookup` and `dir_link`. In dir_lookup, we looped through the directory entries of a specified directory until we found an entry with a name that matched the name we were looking for. In dir_link, we used dir_lookup to find the specified sub-directory and then wrote a new directory entry into it. After writing `dir_lookup` and `dir_link`, we implemented `skippelem` and `namex` in path.c. In `skippelem`, we parsed the necessary information from the specified path string to extract the next path element as well as the remaining path string. In `namex`, we found the corresponding inode for the path specified by parsing each element of the path string passed in using `skipelem`. Next, we altered the `sys_link`, `sys_unlink`, `sys_open`, `sys_mkdir`, and `sys_chdir` system calls by adding additional argument(s) in their int system calls corresponding to the string length of the user input. Next, we wrote the functions `fdalloc`, `sys_read`, `sys_write`, `sys_close`, and `fstat`. In `fdalloc`, we simply loop through the current threads open files to find an available file descriptor. We allocate the file descriptor to the specified file and then return the found file descriptor. In sys_read, we read x bytes into our global kernel buffer using `file_read` and then copy it into the user buffer with `pt_copyout`. In `sys_write`, we first copy in the user data into the kernel buffer using `pt_copyin` and then write to the specified file using `file_write`. In `sys_fstat`, we simply populate the file stat struct passed in using `pt_copyout`. Lastly, in exercise 15 we added spinlocks for `sys_read` and `sys_write` in critical sections where they operate on the global kernel buffer.

# Part 4: Shell

In this part of the lab we implemented a unix-like shell on top of our file system implementation. We first started by creating a new user program in user/shell called shell.c We first added the appropriate functionality to spawn this process by altering sys_spawn and kern/init/init.c Afterwards, we created implemented `sys_readline`, a system call to read in user input. This system call is essentially a wrapper for readline function in kern/dev/console.c. Next, we implemented a user input parser called `findNextArg` which takes in a pointer to a location within the user input returns the next user argument. Finally, we implemented the functionality for the following set of commands: `ls`, `pwd`, `cd`, `cp`, `mv`, `rm`, `mkdir`, `cat`, `touch`, writing a string to a file, and appending a string to a file (we decided to call this command `echo`).

## Test Run
### Commands
```
ls
mkdir dir1
mkdir dir1/dir2
ls
cp README dir1/dir2/COPY
cp -r dir1 dir3
ls
pwd
ls dir3
mv dir3 dir4
ls
cd dir4/dir2
ls
echo "hello world" > COPY
cat COPY
pwd
cd ../..
ls
cd dir1
cd dir2
ls
echo "goodbye world!" >> COPY
cat COPY
touch TOUCH
ls
mv TOUCH /TOUCH
cd
pwd
echo "blah blah blah" > TOUCH
cp TOUCH TOUCH2
ls
rm -r dir1
ls
rm TOUCH
ls
cat TOUCH2
pwd
```
### Output
```
$ ls
.	..	README	
$ mkdir dir1
$ mkdir dir1/dir2
$ ls
.	..	README	dir1	
$ cp README dir1/dir2/COPY
$ cp -r dir1 dir3
$ ls
.	..	README	dir1	dir3	
$ pwd
/
$ ls dir3
.	..	dir2	
$ mv dir3 dir4
$ ls
.	..	README	dir1	dir4	
$ cd dir4/dir2
$ ls
.	..	COPY	
$ echo "hello world" > COPY
$ cat COPY
hello world
$ pwd
/dir4/dir2/
$ cd ../..
$ ls
.	..	README	dir1	dir4	
$ cd dir1
$ cd dir2
$ ls
.	..	COPY	
$ echo "goodbye world!" >> COPY
$ cat COPY
<Original content of README, omitted in this demo output for conciseness>
goodbye world!
$ touch TOUCH
$ ls
.	..	COPY	TOUCH	
$ mv TOUCH /TOUCH
$ cd
$ pwd
/
$ echo "blah blah blah" > TOUCH
$ cp TOUCH TOUCH2
$ ls
.	..	README	dir1	TOUCH	dir4	TOUCH2	
$ rm -r dir1
$ ls
.	..	README	TOUCH	dir4	TOUCH2	
$ rm TOUCH
$ ls
.	..	README	dir4	TOUCH2	
$ cat TOUCH2
blah blah blah
$ pwd
/
```

## Specification

This section specifies the supported functionality of our shell. Unless otherwise specified, paths used in the following section such as `dir`, `file`, `src`, or `dest` can be a full path (starting with `/`) or a path relative to the current working directory. Leading and trailing spaces, as well as extra spaces between arguments, are ignored by our parser so that syntax errors do not crash our shell.

### ls
- `ls dir` will print all directory entries in `dir`, separated by tabs, to the console.
- `ls` with no arguments will list directory entries in the current working directory
### pwd
- `pwd` prints the full path of the current working directory to the console.
### cd 
- `cd dir` will change directory into the directory specified by `dir`.
- `cd` with no arguments will change directory to the home/root directory, `/`.
### cp
- `cp src dest` will make a copy of the file `src` and store the copy as `dest`. `src` must be a valid path and must point to a file. `dest` must not already exist. For instance, if our current working directory contains only the file `README`, `cp README COPY` will create a new file named `COPY` in the current working directory whose content is identical to `README`.
- `cp -r src dest` will recursively copy the folder `src` and store the copy as `dest`. `src` must be a valid path and must point to a directory. `dest` must not already exist.
### mv
- `mv src dest` will rename the file `src` as `dest` (alternatively, the operation can be conceived as moving the file `src` to `dest`). `src` must be a valid path and must point to a file. `dest` must not already exist.
### rm
- `rm file` will remove `file` from the filesystem if it exists.
- `rm -r dir` will recursively remove the directory `dir`, including all of its contents, from the filesystem if it exists.
### mkdir
- `mkdir dir` will create a new directory `dir`, if `dir` does not already exist.
### cat
- `cat file` will print the content of `file` to the console. `file` must be a valid path and must point to a file.
### touch
- `touch file` will create a new empty file with name `file`, if `file` does not already exist.
### echo
- `echo "text content" > file` prints out "text content" to the file with the path `file`. `echo` will create the file if it does not already exist. If the file does already exist, the command will erase the existing content of the file before writing "text content". `echo` will additionally append a newline at the end of the file.
- `echo "text content" >> file` prints out "text content" to the file with the path `file`. `echo` will create the file if it does not already exist. If the file does already exist, its existing content will be kept, and `echo` will append "text content" to the end of the file. `echo` will additionally append a newline at the end of the file.

## Implementation

### ls
For the ls command, we loop through the specified path directory's entries and print out the name of each directory entry. We implemented both support for ls without arguments and ls with arguments (see specification for expected behavior of both).

### pwd
For the pwd command, we implemented a new system call called `sys_getcwd`. This system call iteratively adds the current directory's name and traverses to the root parent directory. As we perform the traversal, we add the name of the current working directory to the kernel buffer and thus we also used added spin locks to our implementation of this function. When we reach the root node, we reverse the kernel buffer as the pwd is printed from parent to child and we use `KERN_INFO` to print the pwd within the kernel.
In order to find the name of a child directory we looped through the `dirent`s of the parent directory (which we obtained by using `dir_lookup` on the ".." directory for the child) until we found the directory entry with the child's inum.

### cd
For the cd command, we simply called the system call chdir with the specified path directory. We implemented functionality for `cd dir` and `cd` without arguments (see specification for expected behavior of both).

### cp
For the cp command, we implemented the functionality to support both copying from an existing file to a new file and copying from an existing directory to a new directory. 
For copying an existing file to a new file, we first create a new destination file if it didn't already exist. Then, we proceed to copy 512 bytes from the source file to the destination file using the read and write system calls. Again, we only copy 512 bytes 
over at a time as our kernel buffer size is of a limited capacity (10000 bytes) and we want to prevent the user from writing beyond this buffer. We implemented copying from an existing directory a new directory recursively. To do this, we first checked if the destination directory existed or not. If not, we created a new destination directory using the mkdir system call. Afterwards, we looped through the entries of our parent directory and called our copy function on each entry. Within our copy function, if the directory entry passed in is a file, then we simply create a new file and copy the file from the source directory into it using the read and write system calls. Otherwise, if the entry is a directory we create a new directory and continue to call our copy function on the directory entry.

### rm
For the rm command, we implemented the functionality to support both removing a single file and removing an entire directory recursively. In both cases, we used the recursive `remove` function that we implemented which takes in a relative path. The base case for our remove function is when the relative path is a file, in which case we simply unlink the file and close it. Otherwise, we first change into the specified path directory and loop through its directory entries and call our remove function on the entries (and we are careful to chdir back out when we are done, so that the current working directory is always the same as before we called `remove`).

### mv

For the mv command, we simply create a link between the source file and the destination file and then remove the link from the source file. We do this using the sys_link and sys_unlink system calls.

### mkdir
For the mkdir command, we first check if the directory specified by the user exists or not using the open system call. If it exists then we don't do anything, if it doesn't exist then we create a new directory with the mkdir system call. 

### cat
For the cat command, we open the specified file from the user input and then read the data 512 bytes at a time into a locally allocated buffer and then print it. We only read 512 bytes at a time because our kernel buffer is 10000 bytes long and so we want to prevent the user from overflowing the kernel buffer and writing into kernel memory.

### touch
For the touch command, we simply call the `open` system call with the `O_CREATE` flag to create a new file. If the file already exists this command does nothing, which is in accordance with the linux touch command.

### echo "some content" > file (file writing)
The command we chose to support writing a string into a file was `echo "string" > filename`. To implement this command, we first opened the filename specified and wrote the content specified by the user 512 bytes at a time. Again, we only write 512 bytes to it at a time as the kernel buffer has a capacity of 10000 bytes and we want to prevent the user from overflowing the kernel buffer and writing into kernel memory. If the file already exists the existing content of the file is removed before we write to it.

### echo "some content" >> file (file appending)
The command we chose to support appending a string to a file was `echo "string" >> filename`. To implement this command, we first check if the file already exists. If it doesn't exist we proceed to create a file. Afterwards we perform a series of "dummy reads" until the end of the file so that we can alter the end of the file and add the specified user content to it using the `write` system call. 