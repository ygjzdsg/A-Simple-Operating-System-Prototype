#include <os/list.h>
#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <screen.h>
#include <stdio.h>
#include <assert.h>
#include <mailbox.h>
#include <os/string.h>
#include <os/smp.h>
#include <pgtable.h>
#include <user_programs.h>
#include <os/elf.h>

pcb_t pcb[NUM_MAX_TASK];

const ptr_t pid0_stack_core0 = INIT_KERNEL_STACK_CORE0 + PAGE_SIZE;
pcb_t pid0_pcb_core0 = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack_core0,
    .user_sp = (ptr_t)pid0_stack_core0,
    .preempt_count = 0,
    .mask = 3
};
const ptr_t pid0_stack_core1 = INIT_KERNEL_STACK_CORE1 + PAGE_SIZE;
pcb_t pid0_pcb_core1 = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack_core1,
    .user_sp = (ptr_t)pid0_stack_core1,
    .preempt_count = 0,
    .mask = 3
}; 

LIST_HEAD(ready_queue);
LIST_HEAD(sleep_queue);


/* current running task PCB */
pcb_t * volatile current_running[NR_CPUS];
extern void ret_from_exception();

pid_t process_id = 0;
extern pid_t thread_id = 1;
void do_scheduler(void)
{
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    // TODO schedule
    // Modify the current_running pointer.
    pcb_t *previous_running;
    previous_running=current_running[cpu_id];

    timer_check();

    if(current_running[cpu_id]->status == TASK_RUNNING){
        current_running[cpu_id]->status=TASK_READY;
        if(current_running[cpu_id]->pid!=0){
            add_element(&current_running[cpu_id]->list,&ready_queue);
        }
    }

    if(!is_empty(&ready_queue)){
        list_node_t *check = ready_queue.next;
        pcb_t *check_pcb;
        list_node_t *check_next;
        while((check != &ready_queue)){
            check_pcb = list_entry(check,pcb_t,list);
            check_next=check->next;
            if((check_pcb->mask==3) || (check_pcb->mask==cpu_id+1)){
                current_running[cpu_id] = check_pcb;
                delete_element(check);
                break;
            }
            check=check_next;
        }
    }else{
        if(cpu_id==0){
            current_running[cpu_id] = &pid0_pcb_core0;
        }else{
            current_running[cpu_id] = &pid0_pcb_core1;
        }
    }

    current_running[cpu_id]->status=TASK_RUNNING;
    if (previous_running->pgdir != current_running[cpu_id]->pgdir){
        set_satp(SATP_MODE_SV39, BOOT_ASID, (uint64_t)(kva2pa(current_running[cpu_id]->pgdir)) >> 12);
        local_flush_tlb_all();
    }
    //set_satp(SATP_MODE_SV39, BOOT_ASID, (uint64_t)(kva2pa(current_running[cpu_id]->pgdir)) >> 12);
    //local_flush_tlb_all();
    // restore the current_runnint's cursor_x and cursor_y
    vt100_move_cursor(current_running[cpu_id]->cursor_x,
                      current_running[cpu_id]->cursor_y);
    //screen_cursor_x = current_running[cpu_id]->cursor_x;
    //screen_cursor_y = current_running[cpu_id]->cursor_y;
    // TODO: switch_to current_running
    
    switch_to(previous_running, current_running[cpu_id]);
}

void do_sleep(uint32_t sleep_time)
{
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    // TODO: sleep(seconds)
    // note: you can assume: 1 second = `timebase` ticks
    // 1. block the current_running
    // 2. create a timer which calls `do_unblock` when timeout
    // 3. reschedule because the current_running is blocked.
    delete_element(&current_running[cpu_id]->list);
    
    current_running[cpu_id]->status=TASK_BLOCKED;
    current_running[cpu_id]->timer=get_ticks() + sleep_time*time_base;
    add_element(&current_running[cpu_id]->list,&sleep_queue);
    do_scheduler();

}

void do_block(list_node_t *pcb_node, list_head *queue)
{
    // TODO: block the pcb task into the block queue
    
    list_entry(pcb_node, pcb_t, list)->status = TASK_BLOCKED;
    add_element(pcb_node,queue);
    do_scheduler();
}

void do_unblock(list_node_t *pcb_node)
{
    // TODO: unblock the `pcb` from the block queue
    list_entry(pcb_node, pcb_t, list)->status = TASK_READY;
    delete_element(pcb_node);
    add_element(pcb_node,&ready_queue);

}


void timer_check(void){
    list_node_t *check = sleep_queue.next;
    pcb_t *check_pcb;
    list_node_t *check_next;
    while((check != &sleep_queue)){
        check_pcb = list_entry(check,pcb_t,list);
        check_next=check->next;
        if(get_ticks() >= check_pcb->timer){
            do_unblock(check);
        }
        check=check_next;
    }
}


pid_t do_spawn(task_info_t *task, void* arg, spawn_mode_t mode){
    extern pid_t process_id;

    int i;
    for (i=0;i<NUM_MAX_TASK;i++){
        if (pcb[i].pid==-1){
            break;
        }
    }
        
    pcb[i].pid=++process_id;
    pcb[i].kernel_sp=pcb[i].kernel_stack_base;
    pcb[i].user_sp=pcb[i].user_stack_base;
    pcb[i].type=task->type;
    pcb[i].mode=mode;
    pcb[i].status=TASK_READY;
    
    pcb[i].cursor_x=0;
    pcb[i].cursor_y=0;
    pcb[i].preempt_count=0;
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    pcb[i].mask=current_running[cpu_id]->mask;
    init_list(&pcb[i].lock_list);
    init_list(&pcb[i].wait_list);

    init_pcb_stack(pcb[i].kernel_sp, pcb[i].user_sp, task->entry_point, &pcb[i], NULL,NULL);
    pcb[i].kernel_sp=pcb[i].kernel_sp-sizeof(regs_context_t)-sizeof(switchto_context_t); 

    add_element(&pcb[i].list,&ready_queue);
    return process_id;
}

int do_kill(pid_t pid){
    
    int i;
    for(i=0;i< NUM_MAX_TASK;i++){
        if (pcb[i].pid!=-1 && pcb[i].pid == pid)
            break;
    }   
    if(i==NUM_MAX_TASK)
        return 0;
    
    if (pcb[i].mode == ENTER_ZOMBIE_ON_EXIT)
        pcb[i].status = TASK_ZOMBIE;
    else
        pcb[i].status = TASK_EXITED;
    
    delete_element(&(pcb[i].list));
    pcb[i].pid==-1;
    
    while(pcb[i].locknum!=-1){
        do_mutex_lock_release(&(whole_locks[pcb[i].lockid[pcb[i].locknum]]));
        pcb[i].locknum--;
    }
    
    for(int i=0;i<5;i++){
        pcb[i].lockid[i]=-1;
    }
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();

    if (pid == current_running[cpu_id]->pid)
        do_scheduler();
    return 1;
}

int do_waitpid(pid_t pid){
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    int i;
    for (i=0;i<NUM_MAX_TASK;i++)
        if (pcb[i].pid!=-1 && pcb[i].pid==pid)
            break;
    if(i==NUM_MAX_TASK)
        return 0;

    if (pcb[i].status!=TASK_ZOMBIE && pcb[i].status!=TASK_EXITED)
        do_block(&(current_running[cpu_id]->list), &(pcb[i].wait_list));

    return 1;
}

void do_exit(void){
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    if (current_running[cpu_id]->mode == ENTER_ZOMBIE_ON_EXIT)
        current_running[cpu_id]->status = TASK_ZOMBIE;
    else
        current_running[cpu_id]->status = TASK_EXITED;

    delete_element(&current_running[cpu_id]->list);
    current_running[cpu_id]->pid=-1;

    while(current_running[cpu_id]->locknum!=-1){
        do_mutex_lock_release(&(whole_locks[current_running[cpu_id]->lockid[current_running[cpu_id]->locknum]]));
        current_running[cpu_id]->locknum--;
    }
    
    for(int i=0;i<5;i++){
        current_running[cpu_id]->lockid[i]=-1;
    }
    
    do_scheduler();
}

void do_process_show(){
    prints("[PROCESS TABLE]\n\r");
    prints("[0] PID : 0 STATUS : RUNNING MASK : 0x3\n\r");
    int j=1;
    for(int i=0;i<NUM_MAX_TASK;i++){
        if(pcb[i].pid!=-1){
            if(pcb[i].status==TASK_RUNNING){
                int core_num;
                if(&pcb[i]==current_running[0]){
                    core_num=0;
                }else{
                    core_num=1;
                }
                prints("[%d] PID : %d STATUS : RUNNING MASK : 0x%d on Core %d\n\r",j,pcb[i].pid,pcb[i].mask,core_num);
                j++;
            }else if(pcb[i].status==TASK_READY){
                prints("[%d] PID : %d STATUS : READY MASK : 0x%d\n\r",j,pcb[i].pid,pcb[i].mask);
                j++;
            }else if(pcb[i].status==TASK_BLOCKED){
                prints("[%d] PID : %d STATUS : BLOCKED MASK : 0x%d\n\r",j,pcb[i].pid,pcb[i].mask);
                j++;
            }else if(pcb[i].status==TASK_ZOMBIE){
                prints("[%d] PID : %d STATUS : ZOMBIE MASK : 0x%d\n\r",j,pcb[i].pid,pcb[i].mask);
                j++;
            }
        }
    }
}

pid_t do_getpid(){
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    return current_running[cpu_id]->pid;
}

//------------------mthread-----------------

int sys_mthread_mutex_init(void* handle)
{
    /* TODO: */
    int *id = (int*)handle;
    *id = mutex_get((int)handle);
    return id;
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

int sys_mthread_barrier_init(void* handle, unsigned count){

    int *id = (int*)handle;
    *id = barrier_get((int)handle,count);
    return id;
 
}

int sys_mthread_barrier_wait(void* handle){
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
     int id = *(int*)handle%NUM_MAX_TASK;
    if (!whole_barriers[id].valid)
        return -1;

    whole_barriers[id].cur_num++;
    if (whole_barriers[id].cur_num == whole_barriers[id].goal){
        while (!is_empty(&(whole_barriers[id].barrier_queue))){
            do_unblock(whole_barriers[id].barrier_queue.next);
        }
        whole_barriers[id].cur_num = 0;
    }else{
        do_block(&(current_running[cpu_id]->list), &(whole_barriers[id].barrier_queue));
    }
        
    return 0;

}

int sys_mthread_barrier_destroy(void* handle){
    int id = *(int*)handle%NUM_MAX_TASK;
    if (!whole_barriers[id].valid){
        return -1;
    }
    whole_barriers[id].valid = 0;
    return 0;

}

int sys_mthread_semaphore_init(void *handle, int val){
    int *id = (int*)handle;
    *id = semaphore_get((int)handle,val);
    return id;
}

int sys_mthread_semaphore_up(void *handle){
    int id = *(int*)handle%NUM_MAX_TASK;
    if (!whole_semaphores[id].valid){
        return -1;
    }

    whole_semaphores[id].sem++;
    if (whole_semaphores[id].sem == 1){
        while (!is_empty(&(whole_semaphores[id].sempahore_queue))){
            do_unblock(whole_semaphores[id].sempahore_queue.next);  
        }
    }
    return 0;
}

int sys_mthread_semaphore_down(void *handle){
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();

    int id = *(int*)handle%NUM_MAX_TASK;
    if (!whole_semaphores[id].valid){
        return -1;
    }
        
    while (whole_semaphores[id].sem<1){
        do_block(&(current_running[cpu_id]->list),&(whole_semaphores[id].sempahore_queue));
    }
        
    whole_semaphores[id].sem--;
}

int sys_mthread_semaphore_destroy(void *handle){
    int id = *(int*)handle%NUM_MAX_TASK;
    if (!whole_semaphores[id].valid)
        return -1;

    whole_semaphores[id].valid = 0;
    return 0;
}

//-------mailbox--------
int do_mbox_open(char *name)
{
    int i;
    for(i=0;i<NUM_MAX_TASK;i++){
        if(whole_mailboxes[i].valid==1){
            if(kstrcmp(name,whole_mailboxes[i].name)==0){
                return i;
            }
        }
            
    }
    for(i=0;i<NUM_MAX_TASK;i++){
        if(whole_mailboxes[i].valid==0){
            whole_mailboxes[i].valid=1;
            kstrcpy(whole_mailboxes[i].name,name);
            whole_mailboxes[i].index = 0;
            for (int j = 0; j < NUM_MAX_TASK; j++)
                whole_mailboxes[i].msg[j] = 0;
            init_list(&whole_mailboxes[i].empty_queue);
            init_list(&whole_mailboxes[i].full_queue);
            do_mutex_lock_init(&mailbox_locks[i]);
            return i;
        }
    }
    prints("No mailbox is available\n");
    return -1;
}

void do_mbox_close(int id){
    whole_mailboxes[id].valid = 0;
}

int do_mbox_send(int id, void *msg, int msg_length){

    int blocked=0;
    do_mutex_lock_acquire(&mailbox_locks[id]);
    
    while((whole_mailboxes[id].index+msg_length)>MAX_MBOX_LENGTH){  
        blocked++;
        do_mutex_lock_release(&mailbox_locks[id]);
        uint64_t cpu_id;
        cpu_id = get_current_cpu_id();
        do_block(&current_running[cpu_id]->list,&(whole_mailboxes[id].full_queue));
        do_mutex_lock_acquire(&mailbox_locks[id]);
    } 

    int i;
    for(i=0;i<msg_length;i++){
        whole_mailboxes[id].msg[whole_mailboxes[id].index++] = ((char*)msg)[i];
    }
    while(!is_empty(&whole_mailboxes[id].empty_queue)){
        do_unblock(whole_mailboxes[id].empty_queue.next);
    } 
    do_mutex_lock_release(&mailbox_locks[id]);
    return blocked;
}

int do_mbox_recv(int id, void *msg, int msg_length){
    int blocked=0;
    do_mutex_lock_acquire(&mailbox_locks[id]);
     while((whole_mailboxes[id].index-msg_length)<0){   
        blocked++; 
        do_mutex_lock_release(&mailbox_locks[id]);
        uint64_t cpu_id;
        cpu_id = get_current_cpu_id();
        do_block(&current_running[cpu_id]->list,&(whole_mailboxes[id].empty_queue));
        do_mutex_lock_acquire(&mailbox_locks[id]);
    } 


    int i,j;
    for(i=0;i<msg_length;i++){
        ((char*)msg)[i]=whole_mailboxes[id].msg[i];
    }
    whole_mailboxes[id].index = whole_mailboxes[id].index - msg_length;
    for(i=0,j=msg_length;i<whole_mailboxes[id].index;i++,j++){
        whole_mailboxes[id].msg[i] = whole_mailboxes[id].msg[j];
    }
    for(;i<64;i++){
        whole_mailboxes[id].msg[i] = 0;
    }

    while(!is_empty(&whole_mailboxes[id].full_queue)){
        do_unblock(whole_mailboxes[id].full_queue.next);
    }
    do_mutex_lock_release(&mailbox_locks[id]);
    return blocked;
}


void do_taskset_p(int mask, pid_t pid){
    int i;
    for (i=0;i<NUM_MAX_TASK;i++)
        if (pcb[i].pid!=-1 && pcb[i].pid==pid)
            break;
    if(i==NUM_MAX_TASK)
        return 0;
        
    pcb[i].mask = mask;
}

void do_taskset(int mask, task_info_t *task, spawn_mode_t mode){
    int i;
    for (i=0;i<NUM_MAX_TASK;i++){
        if (pcb[i].pid==-1){
            break;
        }
    }
        
    pcb[i].pid=++process_id;
    pcb[i].kernel_sp=pcb[i].kernel_stack_base;
    pcb[i].user_sp=pcb[i].user_stack_base;
    pcb[i].type=task->type;
    pcb[i].mode=mode;
    pcb[i].status=TASK_READY;
    
    pcb[i].cursor_x=0;
    pcb[i].cursor_y=0;
    pcb[i].preempt_count=0;
    pcb[i].mask=mask;
    init_list(&pcb[i].lock_list);
    init_list(&pcb[i].wait_list);

    init_pcb_stack(pcb[i].kernel_sp, pcb[i].user_sp, task->entry_point, &pcb[i], NULL,NULL);
    pcb[i].kernel_sp=pcb[i].kernel_sp-sizeof(regs_context_t)-sizeof(switchto_context_t); 

    add_element(&pcb[i].list,&ready_queue);
  //  return process_id;
}

int do_mulmail(int id, void *msg, int msg_length){
    int i,j;

    do_mutex_lock_acquire(&mailbox_locks[id]);

        if((whole_mailboxes[id].index-msg_length)>=0){
            
            for(i=0;i<msg_length;i++){
                ((char*)msg)[i]=whole_mailboxes[id].msg[i];
            }
            whole_mailboxes[id].index = whole_mailboxes[id].index - msg_length;
            for(i=0,j=msg_length;i<whole_mailboxes[id].index;i++,j++){
                whole_mailboxes[id].msg[i] = whole_mailboxes[id].msg[j];
            }
            for(;i<64;i++){
                whole_mailboxes[id].msg[i] = 0;
            }

            while(!is_empty(&whole_mailboxes[id].full_queue)){
                do_unblock(whole_mailboxes[id].full_queue.next);
            }
            do_mutex_lock_release(&mailbox_locks[id]);
            return 2;
        }else{
            
            if((whole_mailboxes[id].index+msg_length)<=MAX_MBOX_LENGTH){
                for(i=0;i<msg_length;i++){
                    whole_mailboxes[id].msg[whole_mailboxes[id].index++] = ((char*)msg)[i];
                }
                while(!is_empty(&whole_mailboxes[id].empty_queue)){
                    do_unblock(whole_mailboxes[id].empty_queue.next);
                } 
                do_mutex_lock_release(&mailbox_locks[id]);
                return 1;
            }else{
                while((whole_mailboxes[id].index+msg_length)>MAX_MBOX_LENGTH){  
                    do_mutex_lock_release(&mailbox_locks[id]);
                    uint64_t cpu_id;
                    cpu_id = get_current_cpu_id();
                    do_block(&current_running[cpu_id]->list,&(whole_mailboxes[id].full_queue));
                    do_mutex_lock_acquire(&mailbox_locks[id]);
                }
        
                for(i=0;i<msg_length;i++){
                    whole_mailboxes[id].msg[whole_mailboxes[id].index++] = ((char*)msg)[i];
                }
                while(!is_empty(&whole_mailboxes[id].empty_queue)){
                    do_unblock(whole_mailboxes[id].empty_queue.next);
                } 
                do_mutex_lock_release(&mailbox_locks[id]);
                return 1;
            }
            
            
            while((whole_mailboxes[id].index-msg_length)<0){   
                do_mutex_lock_release(&mailbox_locks[id]);
                uint64_t cpu_id;
                cpu_id = get_current_cpu_id();
                do_block(&current_running[cpu_id]->list,&(whole_mailboxes[id].empty_queue));
                do_mutex_lock_acquire(&mailbox_locks[id]);
            } 

            for(i=0;i<msg_length;i++){
                ((char*)msg)[i]=whole_mailboxes[id].msg[i];
            }
            whole_mailboxes[id].index = whole_mailboxes[id].index - msg_length;
            for(i=0,j=msg_length;i<whole_mailboxes[id].index;i++,j++){
                whole_mailboxes[id].msg[i] = whole_mailboxes[id].msg[j];
            }
            for(;i<64;i++){
                whole_mailboxes[id].msg[i] = 0;
            }

            while(!is_empty(&whole_mailboxes[id].full_queue)){
                do_unblock(whole_mailboxes[id].full_queue.next);
            }
            do_mutex_lock_release(&mailbox_locks[id]);
            return 2;
        }

    
    return 0;
}

uintptr_t alloc_page_helper_user(uintptr_t kva, uintptr_t pgdir)
{
    return alloc_page_helper(kva, pgdir, 1);
}

void do_show_exec(){
    for (int i=1;i<3;i++)
        prints("%s ", elf_files[i].file_name);
    prints("\n");
}

pid_t do_exec(const char *file_name, int argc, char* argv[], spawn_mode_t mode){
    extern pid_t process_id;
    int i,prog_id;
    for (i=1;i<3;i++){
        if (kstrcmp(file_name, elf_files[i].file_name) == 0)
            break;
    }
    
    if(i==3){
        prints("Not Found\n");
        return -1;
    }
    prog_id=i;
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();

    for (i=0;i<NUM_MAX_TASK;i++){
        if (pcb[i].pid==-1){
            break;
        }
    }
    
    pcb[i].status = TASK_READY;
    pcb[i].type = USER_PROCESS;
    pcb[i].pid=++process_id;
    pcb[i].mode=mode;
    pcb[i].cursor_x=0;
    pcb[i].cursor_y=0;
    pcb[i].preempt_count=0;
    pcb[i].mask=current_running[cpu_id]->mask;
    init_list(&pcb[i].lock_list);
    init_list(&pcb[i].wait_list);
    pcb[i].pnum = 2;


    pcb[i].kernel_sp = pcb[i].kernel_stack_base;
    pcb[i].user_sp = pcb[i].user_stack_base;
    ptr_t user_sp = pcb[i].user_sp;

    ptr_t entry_point = (ptr_t)load_elf(elf_files[prog_id].file_content,
                                      *elf_files[prog_id].file_length,
                                      pcb[i].pgdir,
                                      alloc_page_helper_user);
/*     printk("argc = %d\n", argc);
	for (int b=0; b < argc; ++b) {
	 	printk("argv[%d] = %s\n", b, argv[b]);
	} */

    uintptr_t new_argv = USER_STACK_ADDR - 0xc0;
    uint64_t *kargv = user_sp;
    for (int j = 0;j<argc; j++){
        *(kargv+j)= (uint64_t)new_argv+0x10*(j+1);
        kmemcpy((uint64_t)user_sp + 0x10*(j + 1), argv[j], /* 0x10*(j+1) */0x10);
    }

    init_pcb_stack(pcb[i].kernel_sp,pcb[i].user_sp, (ptr_t)entry_point,&pcb[i],argc,new_argv);
    pcb[i].kernel_sp=pcb[i].kernel_sp-sizeof(regs_context_t)-sizeof(switchto_context_t); 

    pcb[i].user_sp = USER_STACK_ADDR - 0xc0;
    //pcb[i].pgdir = kva2pa(pcb[i].pgdir);


   add_element(&pcb[i].list,&ready_queue);
   return process_id;



}

int do_binsemget(int key){
    int id = key%16;
    if(whole_locks[id].valid == 1){
      return id;
    }
    do_mutex_lock_init(&whole_locks[id]);
    whole_locks[id].valid = 1;
    return id;
}

int do_binsemop(int binsem_id, int op){
    return mutex_op(binsem_id,op);
}

int do_mthread_create(int *thread,void (*start_routine)(void*),void *arg){
    extern pid_t process_id;
    int i;

    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();

    for (i=0;i<NUM_MAX_TASK;i++){
        if (pcb[i].pid==-1){
            break;
        }
    }
    
    pcb[i].pgdir = current_running[cpu_id]->pgdir;
    pcb[i].kernel_stack_base = (uint64_t)alloc_page_helper(KERNEL_STACK_ADDR + PAGE_SIZE, pcb[i].pgdir, 1) ;
    pcb[i].user_stack_base = (uint64_t)alloc_page_helper(USER_STACK_ADDR + PAGE_SIZE, pcb[i].pgdir, 1) ;

    pcb[i].status = TASK_READY;
    pcb[i].type = USER_PROCESS;
    pcb[i].pid=++process_id;
    pcb[i].mode=current_running[cpu_id]->mode;
    pcb[i].cursor_x=0;
    pcb[i].cursor_y=0;
    pcb[i].preempt_count=0;
    pcb[i].locknum = -1;
    pcb[i].mask=current_running[cpu_id]->mask;
    init_list(&pcb[i].lock_list);
    init_list(&pcb[i].wait_list);
    pcb[i].pnum = 2;
    
    pcb[i].kernel_sp = pcb[i].kernel_stack_base;
    pcb[i].user_sp = pcb[i].user_stack_base;


    init_pcb_stack(pcb[i].kernel_sp,pcb[i].user_sp, (ptr_t)start_routine,&pcb[i],arg,NULL);
    pcb[i].kernel_sp=pcb[i].kernel_sp-sizeof(regs_context_t)-sizeof(switchto_context_t); 

    regs_context_t *current_regs=(regs_context_t *)(current_running[cpu_id]->kernel_sp + sizeof(switchto_context_t));
    regs_context_t *thread_regs=(regs_context_t *)(pcb[i].kernel_sp + sizeof(switchto_context_t));
    
    thread_regs->regs[3]=current_regs->regs[3];//gp
    
    pcb[i].user_sp = USER_STACK_ADDR + PAGE_SIZE;
    thread_regs->regs[2]=pcb[i].user_sp;

    thread = pcb[i].pid;
    thread_id++;
    add_element(&pcb[i].list,&ready_queue);
   return process_id;
}