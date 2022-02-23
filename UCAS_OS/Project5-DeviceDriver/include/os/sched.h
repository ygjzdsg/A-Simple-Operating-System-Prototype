/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *        Process scheduling related content, such as: scheduler, process blocking,
 *                 process wakeup, process creation, process kill, etc.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#ifndef INCLUDE_SCHEDULER_H_
#define INCLUDE_SCHEDULER_H_
 
#include <type.h>
#include <os/list.h>
#include <os/mm.h>
#include <os/lock.h>

#include <mailbox.h>
#include <os/smp.h>
#include <context.h>

#define NUM_MAX_TASK 16

typedef enum {
    TASK_BLOCKED,
    TASK_RUNNING,
    TASK_READY,
    TASK_ZOMBIE,
    TASK_EXITED,
} task_status_t;

typedef enum {
    ENTER_ZOMBIE_ON_EXIT,
    AUTO_CLEANUP_ON_EXIT,
} spawn_mode_t;

typedef enum {
    KERNEL_PROCESS,
    KERNEL_THREAD,
    USER_PROCESS,
    USER_THREAD,
} task_type_t;

/* Process Control Block */
typedef struct pcb
{
    /* register context */
    // this must be this order!! The order is defined in regs.h
    reg_t kernel_sp;
    reg_t user_sp;

    // count the number of disable_preempt
    // enable_preempt enables CSR_SIE only when preempt_count == 0
    reg_t preempt_count;

    ptr_t kernel_stack_base;
    ptr_t user_stack_base;

    /* previous, next pointer */
    list_node_t list;
    list_head lock_list;
    list_head wait_list;

    /* process id */
    pid_t pid;
    pid_t wait_pid;
    /* kernel/user thread/process */
    task_type_t type;

    /* BLOCK | READY | RUNNING | ZOMBIE */
    task_status_t status;
    spawn_mode_t mode;

    /* cursor position */
    int cursor_x;
    int cursor_y;
    
    uint64_t timer;
    int mask;
    uintptr_t pgdir;

    list_head plist;
    int pnum; 
    int lockid[5];
    int locknum;
} pcb_t;

/* task information, used to init PCB */
typedef struct task_info
{
    ptr_t entry_point;
    task_type_t type;
} task_info_t;

/* ready queue to run */
extern list_head ready_queue;
extern list_head sleep_queue;

/* current running task PCB */
//extern pcb_t * volatile current_running;
extern pcb_t * volatile current_running[NR_CPUS];
extern pid_t process_id;


extern pcb_t pcb[NUM_MAX_TASK];

// extern pcb_t kernel_pcb[NR_CPUS];
extern pcb_t pid0_pcb_core0;
extern const ptr_t pid0_stack_core0;
extern pcb_t pid0_pcb_core1;
extern const ptr_t pid0_stack_core1;

extern void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb, int argc, char *arg[]);
extern void switch_to(pcb_t *prev, pcb_t *next);
extern void do_scheduler(void);
extern void do_sleep(uint32_t);


void do_block(list_node_t *, list_head *queue);
void do_unblock(list_node_t *);

int sys_mthread_mutex_init(void* handle);
int sys_mthread_mutex_lock(void* handle);
int sys_mthread_mutex_unlock(void* handle);

int sys_mthread_barrier_init(void *handle, unsigned count);
int sys_mthread_barrier_wait(void *handle);
int sys_mthread_barrier_destroy(void *handle);

int sys_mthread_semaphore_init(void *handle, int val);
int sys_mthread_semaphore_up(void *handle);
int sys_mthread_semaphore_down(void *handle);
int sys_mthread_semaphore_destroy(void *handle);

void timer_check(void);


extern pid_t do_spawn(task_info_t *task, void* arg, spawn_mode_t mode);
extern void do_exit(void);
extern int do_kill(pid_t pid);
extern int do_waitpid(pid_t pid);
extern void do_process_show();
extern pid_t do_getpid();
extern void do_taskset_p(int mask, pid_t pid);
extern void do_taskset(int mask, task_info_t *task, spawn_mode_t mode);


int do_mbox_open(char *name);
void do_mbox_close(int mailbox_id);
int do_mbox_send(int mailbox_id, void *msg, int msg_length);
int do_mbox_recv(int mailbox_id, void *msg, int msg_length);
int do_mulmail(int mailbox_id, void *msg, int msg_length);

typedef void (*user_entry_t)(unsigned long,unsigned long,unsigned long);
extern pid_t do_exec(const char* file_name, int argc, char* argv[], spawn_mode_t mode);
extern void do_show_exec();

extern uintptr_t alloc_page_helper_user(uintptr_t kva, uintptr_t pgdir);


int do_binsemget(int key);
int do_binsemop(int binsem_id, int op);
int do_mthread_create(int *thread,void (*start_routine)(void*),void *arg);
#endif
