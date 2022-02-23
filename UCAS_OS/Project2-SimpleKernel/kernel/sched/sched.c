#include <os/list.h>
#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <screen.h>
#include <stdio.h>
#include <assert.h>

#include <asm/regs.h>
#include <sbi.h>
pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack = INIT_KERNEL_STACK + PAGE_SIZE;
pcb_t pid0_pcb = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack,
    .user_sp = (ptr_t)pid0_stack,
    .preempt_count = 0
};

LIST_HEAD(ready_queue);
LIST_HEAD(sleep_queue);

/* current running task PCB */
pcb_t * volatile current_running;

/* global process id */
pid_t process_id = 1;

void do_scheduler(void)
{
    // TODO schedule
    // Modify the current_running pointer.
    pcb_t *previous_running;
    list_node_t *p;
    previous_running=current_running;

    if(current_running->status!=TASK_BLOCKED){
        current_running->status=TASK_READY;
        if(current_running->pid!=0){
            current_running->list.priority--;
            p=ready_queue.next;
            while(p!=&ready_queue){
              list_entry(p, pcb_t, list)->list.priority++;
              p=p->next;
            } 
           add_element_priority(&current_running->list,&ready_queue);
        }
    }

    if(is_empty(&ready_queue)==0){
        current_running=list_entry(ready_queue.next, pcb_t, list);
        delete_element(ready_queue.next);
    }
    
    current_running->status=TASK_RUNNING;
    // restore the current_runnint's cursor_x and cursor_y
    vt100_move_cursor(current_running->cursor_x,
                      current_running->cursor_y);
    screen_cursor_x = current_running->cursor_x;
    screen_cursor_y = current_running->cursor_y;
    // TODO: switch_to current_running
    
    switch_to(previous_running, current_running);
}

void do_sleep(uint32_t sleep_time)
{
    // TODO: sleep(seconds)
    // note: you can assume: 1 second = `timebase` ticks
    // 1. block the current_running
    // 2. create a timer which calls `do_unblock` when timeout
    // 3. reschedule because the current_running is blocked.
    delete_element(&current_running->list);
    add_element(&current_running->list,&sleep_queue);
    current_running->status=TASK_BLOCKED;
    current_running->timer = get_ticks() + sleep_time*time_base;
    do_scheduler();

}

void do_block(list_node_t *pcb_node, list_head *queue)
{
    // TODO: block the pcb task into the block queue
  
    add_element(pcb_node,queue);
    do_scheduler();
}

void do_unblock(list_node_t *pcb_node)
{
    // TODO: unblock the `pcb` from the block queue
   
    delete_element(pcb_node);
    add_element_priority(pcb_node,&ready_queue);

}


int sys_mthread_mutex_init(void* handle)
{
    /* TODO: */
    int *id = (int*)handle;
    *id = mutex_get((int)handle);
    return 0;
}

int sys_mthread_mutex_lock(void* handle) 
{
    /* TODO: */
    return mutex_op(*(int*)handle,0);
}

int sys_mthread_mutex_unlock(void* handle)
{
    /* TODO: */
   return mutex_op(*(int*)handle,1);
}

void timer_check(void){
    list_node_t *check = sleep_queue.next;
    pcb_t *check_pcb;
    while(!is_empty(&sleep_queue) && (check != &sleep_queue)){
        check_pcb = list_entry(check,pcb_t,list);
        if(get_ticks() >= check_pcb->timer){
            do_unblock(check);
        }
        check=check->next;
    }
}

void set_priority(int priority){
    current_running->list.priority = priority;
}

int fork(void){
    pcb_t *child = (regs_context_t *)kmalloc(sizeof(pcb_t));
    *child=*current_running;
    child->kernel_sp=allocPage(1)+PAGE_SIZE;
    child->user_sp=allocPage(1)+PAGE_SIZE;
    child->pid=process_id;
    child->status=TASK_READY;
    child->type=USER_THREAD;
    child->cursor_x=0;
    child->cursor_y=0;
    child->preempt_count=0;
    child->list.priority=1;
   
    kmemcpy(child->kernel_sp-PAGE_SIZE,current_running->kernel_sp+sizeof(regs_context_t)+sizeof(switchto_context_t)-PAGE_SIZE,PAGE_SIZE);
    kmemcpy(child->user_sp-PAGE_SIZE,current_running->user_sp-PAGE_SIZE,PAGE_SIZE);

    regs_context_t *pt_regs = (regs_context_t *)(child->kernel_sp - sizeof(regs_context_t));
    pt_regs->regs[4] = (reg_t)child;   //tp
    pt_regs->regs[2] = pt_regs->regs[2] + child->user_sp - current_running->user_sp; //sp
    pt_regs->regs[8] = pt_regs->regs[8] + child->user_sp - current_running->user_sp; //fp

    child->kernel_sp = child->kernel_sp - sizeof(regs_context_t) - sizeof(switchto_context_t);

    add_element_priority(&child->list, &ready_queue);
    process_id++;
    return (process_id-1);
}
