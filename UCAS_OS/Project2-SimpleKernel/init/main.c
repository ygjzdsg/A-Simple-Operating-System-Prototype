/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
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

#include <common.h>
#include <os/irq.h>
#include <os/mm.h>
#include <os/sched.h>
#include <screen.h>
#include <sbi.h>
#include <stdio.h>
#include <os/time.h>
#include <os/syscall.h>
#include <test.h>
#include <os/lock.h>

#include <csr.h>


extern void ret_from_exception();
extern void __global_pointer$();

static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb)
{
    regs_context_t *pt_regs=(regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    
    /* TODO: initialization registers
     * note: sp, gp, ra, sepc, sstatus
     * gp should be __global_pointer$
     * To run the task in user mode,
     * you should set corresponding bits of sstatus(SPP, SPIE, etc.).
     */
    pt_regs->regs[1]=entry_point;  
    pt_regs->regs[2]=user_stack;  
    pt_regs->regs[3]=__global_pointer$;                                                                                                             //ra
    pt_regs->sepc=entry_point;      
    pt_regs->sstatus=SR_SPIE; 

   
    // set sp to simulate return from switch_to
    /* TODO: you should prepare a stack, and push some values to
     * simulate a pcb context.
     */
    switchto_context_t *sw_regs=(switchto_context_t *)(kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t));
    if (pcb->type == KERNEL_PROCESS || pcb->type == KERNEL_THREAD)
        sw_regs->regs[0] = entry_point;
    else
        sw_regs->regs[0] = &ret_from_exception;
    sw_regs->regs[1] = user_stack;                                                        
    
}

static void init_pcb()
{
     /* initialize all of your pcb and add them into ready_queue
     * TODO:
     */
    pid0_pcb.status=TASK_RUNNING;
    
    init_list(&ready_queue);
    int i,j,k;
    process_id=1;
    for (i=0;i<num_fork_tasks;i++,process_id++){

        pcb[i].kernel_sp=allocPage(1)+PAGE_SIZE; 
        pcb[i].user_sp=allocPage(1)+PAGE_SIZE;
        add_element(&pcb[i].list,&ready_queue);

        pcb[i].pid=process_id;
        pcb[i].status=TASK_READY;
        pcb[i].type=fork_tasks[i]->type;
        pcb[i].cursor_x=0;
        pcb[i].cursor_y=0;
        pcb[i].preempt_count=0;
 
        pcb[i].list.priority=fork_tasks[i]->priority;

        init_pcb_stack(pcb[i].kernel_sp,pcb[i].user_sp,fork_tasks[i]->entry_point,&pcb[i]);
        pcb[i].kernel_sp=pcb[i].kernel_sp-sizeof(regs_context_t)-sizeof(switchto_context_t); 

    }
    /* remember to initialize `current_running`
     * TODO:
     */
    current_running = &pid0_pcb;
}

static void init_syscall(void)
{
    // initialize system call table.
   
    syscall[SYSCALL_YIELD] = do_scheduler;
    syscall[SYSCALL_SLEEP] = do_sleep;
    syscall[SYSCALL_WRITE] = screen_write;
    syscall[SYSCALL_CURSOR] = screen_move_cursor;
    syscall[SYSCALL_REFLUSH] = screen_reflush;
    syscall[SYSCALL_GET_TIMEBASE] = get_time_base;         
    syscall[SYSCALL_GET_TICK] = get_ticks;
    syscall[MTHREAD_MUTEX_INIT] = sys_mthread_mutex_init;
    syscall[MTHREAD_MUTEX_LOCK] = sys_mthread_mutex_lock;
    syscall[MTHREAD_MUTEX_UNLOCK] = sys_mthread_mutex_unlock;
    syscall[SYSCALL_READ] = sbi_console_getchar;
    syscall[SYSCALL_FORK] = fork;
    syscall[SYSCALL_SET_PRIORITY] = set_priority;
}

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main()
{
    // init Process Control Block (-_-!)
    init_pcb();
    printk("> [INIT] PCB initialization succeeded.\n\r");

    // read CPU frequency
    time_base=sbi_read_fdt(TIMEBASE);

    // init interrupt (^_^)
    init_exception();
    printk("> [INIT] Interrupt processing initialization succeeded.\n\r");

    // init system call table (0_0)
    init_syscall();
    printk("> [INIT] System call initialized successfully.\n\r");

    // fdt_print(riscv_dtb);

    // init screen (QAQ)
    init_screen();
    printk("> [INIT] SCREEN initialization succeeded.\n\r");

    // TODO:
    // Setup timer interrupt and enable all interrupt
   
    enable_interrupt();
    sbi_set_timer(get_ticks() + time_base / 1000);

    while (1) {
        // (QAQQQQQQQQQQQ)
        // If you do non-preemptive scheduling, you need to use it
        // to surrender control do_scheduler();
        // enable_interrupt();
        // __asm__ __volatile__("wfi\n\r":::);
       // do_scheduler();
       ;
    };
    return 0;
}
