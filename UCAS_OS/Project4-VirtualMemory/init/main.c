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
#include <os/lock.h>
#include <mailbox.h>
#include <csr.h>
#include <os/smp.h>

#include <pgtable.h>
#include <user_programs.h>
#include <os/elf.h>

extern void ret_from_exception();
extern void __global_pointer$();


void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb, int argc, char *argv[])
{
    regs_context_t *pt_regs=(regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    
    pt_regs->regs[1]=entry_point;  
    pt_regs->regs[2] = USER_STACK_ADDR - 0xc0;
    pt_regs->regs[3]=__global_pointer$;
    pt_regs->regs[10] = argc;
    pt_regs->regs[11] = argv;
    pt_regs->sstatus = SR_SPIE | SR_SUM;
    pt_regs->sepc=entry_point;

    

   
    // set sp to simulate return from switch_to
    /* TODO: you should prepare a stack, and push some values to
     * simulate a pcb context.
     */
    switchto_context_t *sw_regs=(switchto_context_t *)(kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t));

    sw_regs->regs[0] = &ret_from_exception; 
    sw_regs->regs[1] = user_stack;
    
}




static void init_pcb()
{
     /* initialize all of your pcb and add them into ready_queue
     * TODO:
     */
    extern pid_t process_id;
    for(int i=0;i<NUM_MAX_TASK;i++){
        pcb[i].pid=-1;
        pcb[i].locknum=-1;
        for(int i=0;i<5;i++){
            pcb[i].lockid[i]=-1;
        }
        init_list(&(pcb[i].wait_list));
        pcb[i].pgdir = allocPage();
        clear_pgdir(pcb[i].pgdir);
        kmemcpy((char *)pcb[i].pgdir, (char *)pa2kva(PGDIR_PA), PAGE_SIZE);
        if(i>0){
           pcb[i].kernel_stack_base = (uint64_t)alloc_page_helper(KERNEL_STACK_ADDR - PAGE_SIZE, pcb[i].pgdir, 0) + PAGE_SIZE;
           pcb[i].user_stack_base = (uint64_t)alloc_page_helper(USER_STACK_ADDR - PAGE_SIZE, pcb[i].pgdir, 1) + PAGE_SIZE - 0xc0;
           init_list(&pcb[i].page_list);
           init_list(&pcb[i].used_page_queue);
        } 
    }  

    pid0_pcb_core0.status=TASK_RUNNING;
    pid0_pcb_core0.pid=0;
    pid0_pcb_core1.status=TASK_RUNNING;
    pid0_pcb_core1.pid=0;

    process_id=0;

  //  clear_pgdir(pcb[0].pgdir);

  //  kmemcpy((char *)pcb[0].pgdir, (char *)pa2kva(PGDIR_PA), PAGE_SIZE);
    
    pcb[0].kernel_stack_base = (uint64_t)alloc_page_helper(KERNEL_STACK_ADDR - PAGE_SIZE, pcb[0].pgdir, 0) + PAGE_SIZE - 128;
    pcb[0].user_stack_base = (uint64_t)alloc_page_helper(USER_STACK_ADDR - PAGE_SIZE, pcb[0].pgdir, 1) + PAGE_SIZE - 128;

    pcb[0].kernel_sp = pcb[0].kernel_stack_base;
    pcb[0].user_sp = pcb[0].user_stack_base;
    
    unsigned file_len = *(elf_files[0].file_length);
    user_entry_t entry_point = (user_entry_t)load_elf(elf_files[0].file_content, file_len, pcb[0].pgdir, alloc_page_helper_user);

    pcb[0].pid=1;
    pcb[0].status=TASK_READY;
    pcb[0].type=USER_PROCESS;
    pcb[0].pagenum=2;

    pcb[0].cursor_x=0;
    pcb[0].cursor_y=0;
    pcb[0].preempt_count=0;
    pcb[0].mask = 3;
    init_list(&(pcb[0].lock_list));
    init_list(&(pcb[0].wait_list));

    init_pcb_stack(pcb[0].kernel_sp,pcb[0].user_sp, (ptr_t)entry_point,&pcb[0],0,NULL);
    pcb[0].kernel_sp=pcb[0].kernel_sp-sizeof(regs_context_t)-sizeof(switchto_context_t); 

    pcb[0].user_sp = USER_STACK_ADDR - 128;
    //pcb[0].pgdir = kva2pa(pcb[0].pgdir);

    add_element(&pcb[0].list,&ready_queue);
    process_id++;

    /* remember to initialize `current_running`
     * TODO:
     */
    current_running[0] = &pid0_pcb_core0;
    current_running[1] = &pid0_pcb_core1;


}



static void init_syscall(void)
{
    // initialize system call table.
    for(int i = 0; i < NUM_SYSCALLS; i++)
		syscall[i] = handle_other;
    syscall[SYSCALL_YIELD]        = do_scheduler;
    syscall[SYSCALL_SLEEP]        = do_sleep;
    syscall[SYSCALL_WRITE]        = screen_write;
    syscall[SYSCALL_CURSOR]       = screen_move_cursor;
    syscall[SYSCALL_REFLUSH]      = screen_reflush;
    syscall[SYSCALL_SCREEN_CLEAR] = screen_clear;
    syscall[SYSCALL_GET_TIMEBASE] = get_time_base;         
    syscall[SYSCALL_GET_TICK]     = get_ticks;
    syscall[MTHREAD_MUTEX_INIT] = sys_mthread_mutex_init;
    syscall[MTHREAD_MUTEX_LOCK] = sys_mthread_mutex_lock;
    syscall[MTHREAD_MUTEX_UNLOCK] = sys_mthread_mutex_unlock;
    syscall[SYSCALL_GET_CHAR] = sbi_console_getchar;
    syscall[SYSCALL_SPAWN] = do_spawn;
    syscall[SYSCALL_EXIT] = do_exit;
    syscall[SYSCALL_KILL] = do_kill;
    syscall[SYSCALL_WAITPID] = do_waitpid;
    syscall[SYSCALL_PS] = do_process_show;
    syscall[SYSCALL_GETPID] = do_getpid;
    syscall[MTHREAD_BARRIER_INIT] = sys_mthread_barrier_init;
    syscall[MTHREAD_BARRIER_WAIT] = sys_mthread_barrier_wait;
    syscall[MTHREAD_BARRIER_DESTROY] = sys_mthread_barrier_destroy;

    syscall[MTHREAD_SEMAPHORE_INIT] = sys_mthread_semaphore_init;
    syscall[MTHREAD_SEMAPHORE_UP] = sys_mthread_semaphore_up;
    syscall[MTHREAD_SEMAPHORE_DOWN] = sys_mthread_semaphore_down;
    syscall[MTHREAD_SEMAPHORE_DESTROY] = sys_mthread_semaphore_destroy;
    syscall[SYSCALL_MAILBOX_OPEN] = do_mbox_open;
    syscall[SYSCALL_MAILBOX_CLOSE] = do_mbox_close;
    syscall[SYSCALL_MAILBOX_SEND] = do_mbox_send;
    syscall[SYSCALL_MAILBOX_RECV] = do_mbox_recv;
    syscall[SYSCALL_MULMAIL] = do_mulmail;

    syscall[SYSCALL_TASKSET_P] = do_taskset_p;
    syscall[SYSCALL_TASKSET]   = do_taskset;
    syscall[SYSCALL_EXEC] = do_exec;
    syscall[SYSCALL_SHOW_EXEC] = do_show_exec;

    syscall[SYSCALL_BINSEMGET] = do_binsemget;
    syscall[SYSCALL_BINSEMOP] = do_binsemop;
    syscall[MTHREAD_CREATE] = do_mthread_create;

}

void cancel_temp_mapping(){

    uint64_t va = 0x50000000;
    uint64_t pgdir = 0xffffffc05e000000;
    uint64_t vpn_2 = (va >> 30) & ~(~0 << 9);
    uint64_t vpn_1 = (va >> 21) & ~(~0 << 9);
    PTE *level_2 = (PTE *)pgdir + vpn_2;
    PTE *map_pte = (PTE *)pa2kva(get_pa(*level_2)) + vpn_1;
    *level_2 = 0;
    *map_pte = 0;
}
// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main()
{
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    // init Process Control Block (-_-!)
    if(cpu_id==0){
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
         for (int i = 0; i < NUM_MAX_TASK; i++)
        {
            whole_locks[i].valid = 0;
            whole_barriers[i].valid = 0;
            init_list(&whole_barriers[i].barrier_queue);
            whole_semaphores[i].valid = 0;
            init_list(&whole_semaphores[i].sempahore_queue);
            whole_mailboxes[i].index = 0;
            whole_mailboxes[i].valid = 0;
            for (int j = 0; j < NUM_MAX_TASK; j++)
                whole_mailboxes[i].msg[j] = 0;
            init_list(&whole_mailboxes[i].empty_queue);
            init_list(&whole_mailboxes[i].full_queue); 
            do_mutex_lock_init(&mailbox_locks[i]);
        } 
        //init_list(&used_page_queue);

/*      smp_init();
        lock_kernel();
        wakeup_other_hart(); */
        lock_kernel();
        cancel_temp_mapping();
  
    }else{
        lock_kernel();
        setup_exception();
    }
    printk("running cpu_id=%d\n",cpu_id);
    sbi_set_timer(get_ticks() + time_base / 500);
    //reset_irq_timer();
    unlock_kernel();
    enable_interrupt();

    while (1) {
        // (QAQQQQQQQQQQQ)
        // If you do non-preemptive scheduling, you need to use it
        // to surrender control do_scheduler();
       //  enable_interrupt();
        // __asm__ __volatile__("wfi\n\r":::);
       // do_scheduler();
    };
    return 0;
}
