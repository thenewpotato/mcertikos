#include <lib/debug.h>
#include <lib/pmap.h>
#include <lib/types.h>
#include <lib/x86.h>
#include <lib/trap.h>
#include <lib/syscall.h>
#include <dev/intr.h>
#include <pcpu/PCPUIntro/export.h>
#include <dev/console.h>
#include <dev/vga.h>
#include <dev/keyboard.h>

#include "import.h"

static char sys_buf[NUM_IDS][PAGESIZE];

// Retval set to 0 is successful, -1 if failed
// Content of readline returned by writing to buffer passsed through syscall arg
void sys_readline(tf_t *tf)
{
//    KERN_INFO("readline enter\n");
    uintptr_t promptUser = syscall_get_arg2(tf);
    size_t promptlen = syscall_get_arg3(tf);
    uintptr_t resultUser = syscall_get_arg4(tf);
//    KERN_INFO("readline prompt=%p, len=%d, result=%p\n", promptUser, promptlen, resultUser);

    // Error checking
    if (promptlen > 128)
    {
        syscall_set_retval1(tf, E_INVAL_ARG);
        syscall_set_errno(tf, -1);
        return;
    }

    char prompt[promptlen + 1];

    // Copy prompt into kernel memory
    if (pt_copyin(get_curid(), promptUser, prompt, promptlen) != promptlen)
    {
        syscall_set_errno(tf, E_MEM);
        syscall_set_retval1(tf, -1);
        return;
    };

    prompt[promptlen] = '\0';

    // readline ensure that the output is at most 1023 bytes (not including \0)
    // and it ensures that the last character is always \0. our string counting loop
    // does not account for the \0 character, which we nonetheless want to copy back
    // to the user
    char *result = readline(prompt);
    // we count the # chars ourselves b/c no access to strlen
    size_t i = 0; // i will count number of characters excl. \0
    while (result[i] != '\0')
    {
        i++;
    }
    size_t resultlen = i + 1;

    // Copy readline result back to user memory
    // Even if the user output is empty, there should still be at least 1 character (\0)
    if (pt_copyout(result, get_curid(), resultUser, resultlen) == 0)
    {
        // TODO: error number
        syscall_set_errno(tf, E_MEM);
        syscall_set_retval1(tf, -1);
    }

    syscall_set_errno(tf, E_SUCC);
    syscall_set_retval1(tf, 0);
}
/**
 * Copies a string from user into buffer and prints it to the screen.
 * This is called by the user level "printf" library as a system call.
 */
void sys_puts(tf_t *tf)
{
    unsigned int cur_pid;
    unsigned int str_uva, str_len;
    unsigned int remain, cur_pos, nbytes;

    cur_pid = get_curid();
    str_uva = syscall_get_arg2(tf);
    str_len = syscall_get_arg3(tf);

    if (!(VM_USERLO <= str_uva && str_uva + str_len <= VM_USERHI))
    {
        syscall_set_errno(tf, E_INVAL_ADDR);
        return;
    }

    remain = str_len;
    cur_pos = str_uva;

    while (remain)
    {
        if (remain < PAGESIZE - 1)
            nbytes = remain;
        else
            nbytes = PAGESIZE - 1;

        if (pt_copyin(cur_pid, cur_pos, sys_buf[cur_pid], nbytes) != nbytes)
        {
            syscall_set_errno(tf, E_MEM);
            return;
        }

        sys_buf[cur_pid][nbytes] = '\0';
        KERN_INFO("%s", sys_buf[cur_pid]);

        remain -= nbytes;
        cur_pos += nbytes;
    }

    syscall_set_errno(tf, E_SUCC);
}

void sys_getc(tf_t *tf) {
    int c;
    while ((c = kbd_proc_data()) == -1) {}
    KERN_INFO("%d\n", c);
    syscall_set_errno(tf, E_SUCC);
    syscall_set_retval1(tf, c);
}

extern uint8_t _binary___obj_user_pingpong_ping_start[];
extern uint8_t _binary___obj_user_pingpong_pong_start[];
extern uint8_t _binary___obj_user_pingpong_ding_start[];
extern uint8_t _binary___obj_user_fstest_fstest_start[];
extern uint8_t _binary___obj_user_shell_shell_start[];

/**
 * Spawns a new child process.
 * The user level library function sys_spawn (defined in user/include/syscall.h)
 * takes two arguments [elf_id] and [quota], and returns the new child process id
 * or NUM_IDS (as failure), with appropriate error number.
 * Currently, we have three user processes defined in user/pingpong/ directory,
 * ping, pong, and ding.
 * The linker ELF addresses for those compiled binaries are defined above.
 * Since we do not yet have a file system implemented in mCertiKOS,
 * we statically load the ELF binaries into the memory based on the
 * first parameter [elf_id].
 * For example, ping, pong, and ding correspond to the elf_ids
 * 1, 2, 3, and 4, respectively.
 * If the parameter [elf_id] is none of these, then it should return
 * NUM_IDS with the error number E_INVAL_PID. The same error case apply
 * when the proc_create fails.
 * Otherwise, you should mark it as successful, and return the new child process id.
 */
void sys_spawn(tf_t *tf)
{
    unsigned int new_pid;
    unsigned int elf_id, quota;
    void *elf_addr;
    unsigned int curid = get_curid();

    elf_id = syscall_get_arg2(tf);
    quota = syscall_get_arg3(tf);

    if (!container_can_consume(curid, quota))
    {
        syscall_set_errno(tf, E_EXCEEDS_QUOTA);
        syscall_set_retval1(tf, NUM_IDS);
        return;
    }
    else if (NUM_IDS < curid * MAX_CHILDREN + 1 + MAX_CHILDREN)
    {
        syscall_set_errno(tf, E_MAX_NUM_CHILDEN_REACHED);
        syscall_set_retval1(tf, NUM_IDS);
        return;
    }
    else if (container_get_nchildren(curid) == MAX_CHILDREN)
    {
        syscall_set_errno(tf, E_INVAL_CHILD_ID);
        syscall_set_retval1(tf, NUM_IDS);
        return;
    }

    switch (elf_id)
    {
    case 1:
        elf_addr = _binary___obj_user_pingpong_ping_start;
        break;
    case 2:
        elf_addr = _binary___obj_user_pingpong_pong_start;
        break;
    case 3:
        elf_addr = _binary___obj_user_pingpong_ding_start;
        break;
    case 4:
        elf_addr = _binary___obj_user_fstest_fstest_start;
        break;
    case 5:
        elf_addr = _binary___obj_user_shell_shell_start;
        break;
    default:
        syscall_set_errno(tf, E_INVAL_PID);
        syscall_set_retval1(tf, NUM_IDS);
        return;
    }

    new_pid = proc_create(elf_addr, quota);

    if (new_pid == NUM_IDS)
    {
        syscall_set_errno(tf, E_INVAL_PID);
        syscall_set_retval1(tf, NUM_IDS);
    }
    else
    {
        syscall_set_errno(tf, E_SUCC);
        syscall_set_retval1(tf, new_pid);
    }
}

/**
 * Yields to another thread/process.
 * The user level library function sys_yield (defined in user/include/syscall.h)
 * does not take any argument and does not have any return values.
 * Do not forget to set the error number as E_SUCC.
 */
void sys_yield(tf_t *tf)
{
    thread_yield();
    syscall_set_errno(tf, E_SUCC);
}

/*
sys_cd
- takes in the directory path to cd to
- 


USER SIDE
function(path, relativePath) => resultingPath

ls
- resolve relative path
- open path
- read dirent entries

pwd
- print out user stored path

cp
- resolve src relative path
- resolve dst relative path
- open dst with create flag
- open src
- get file size of src from fstat
- read from src and write to dst

mv
- resolve src relative path
- resolve dst relative path
- same procedure as cp
- call sys_unlink on src

rm
- resolve relative path
- call sys_unlink

mkdir
- resolve relative path
- use system call to create folder

cat
- resolve relative path
- open path
- get file size from fstat
- read from path until end of file

touch
- resolve relative path
- open path with create flag
- close fd


sys_mkdir(path of folder to create)
- namex lookup parent
- add entry inside parent
*/

#define KERNEL_BUFFER_SIZE 10000
extern char kernel_buffer[KERNEL_BUFFER_SIZE];

void sys_draw(tf_t *tf) {
    uintptr_t loc_ptr = syscall_get_arg2(tf);
    uintptr_t bitmap_ptr = syscall_get_arg3(tf);
    unsigned char color = syscall_get_arg4(tf);

    struct rect_loc loc;
    pt_copyin(get_curid(), loc_ptr, &loc, sizeof(struct rect_loc));
    pt_copyin(get_curid(), bitmap_ptr, kernel_buffer, loc.height * loc.width / 8);
    vga_set_rectangle(loc, kernel_buffer, color);
    syscall_set_errno(tf, E_SUCC);
}

void sys_setvideo(tf_t *tf) {
    unsigned int mode = syscall_get_arg2(tf);
    vga_set_mode(mode);
    syscall_set_errno(tf, E_SUCC);
}


void sys_draw_pixel(tf_t * tf){
    unsigned int row = syscall_get_arg2(tf);
    unsigned int col = syscall_get_arg3(tf);
    unsigned int color = syscall_get_arg4(tf);

    vga_set_pixel(row, col, color);

    syscall_set_errno(tf, E_SUCC);
}