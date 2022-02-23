#include <os/lock.h>
#include <os/sched.h>
#include <os/list.h>
#include <atomic.h>

void do_mutex_lock_init(mutex_lock_t *lock)
{
    /* TODO */

    lock->lock.status = UNLOCKED;
    init_list(&lock->block_queue);
}

void do_mutex_lock_acquire(mutex_lock_t *lock)
{
    /* TODO */
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    while (lock->lock.status == LOCKED)
        do_block(&(current_running[cpu_id]->list), &(lock->block_queue));
   // add_element(&(lock->list), &(current_running[cpu_id]->lock_list));
    
    lock->lock.status = LOCKED;
}

void do_mutex_lock_release(mutex_lock_t *lock)
{
    /* TODO */
    lock->lock.status = UNLOCKED;
    //delete_element(&(lock->list));
    while (!is_empty(&(lock->block_queue)))
        do_unblock(lock->block_queue.next);
        
        
}

int mutex_get(int key){
    int id = key%NUM_MAX_TASK;
    int num=0;
  /*  while (whole_locks[id].valid){
        num++;
        id=(id+1)%NUM_MAX_TASK;
        if(num==NUM_MAX_TASK) 
            return -1;
    } */
    whole_locks[id].valid = 1;
    do_mutex_lock_init(&whole_locks[id]);
    return id;
}

int mutex_op(int handle, int op){
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    int i;
    if(op==0){
        for(i=0;i<5;i++){
            if(current_running[cpu_id]->lockid[i]==handle){
                break;
            }
        }
        if(i==5){
            current_running[cpu_id]->locknum++;
            current_running[cpu_id]->lockid[current_running[cpu_id]->locknum]=handle;
        }

        do_mutex_lock_acquire(&whole_locks[handle]);
    }else{
        for(i=0;i<5;i++){
            if(current_running[cpu_id]->lockid[i]==handle){
                break;
            }
        }
        for(;i<4;i++){
            current_running[cpu_id]->lockid[i]=current_running[cpu_id]->lockid[i+1];
        }
        current_running[cpu_id]->lockid[i]=-1;
        current_running[cpu_id]->locknum--;
        do_mutex_lock_release(&whole_locks[handle]);
    }
    return 0;
}  

int barrier_get(int key,int count){
    int id=key%NUM_MAX_TASK;
    int num=0;
    while (whole_barriers[id].valid){
        num++;
        id=(id+1)%NUM_MAX_TASK;
        if(num==NUM_MAX_TASK) 
            return -1;
    }
    whole_barriers[id].valid = 1;
    whole_barriers[id].goal = count;
    whole_barriers[id].cur_num = 0;
    return 0;
}
int semaphore_get(int key,int val){
    int id=key%NUM_MAX_TASK;
    int num=0;
    while (whole_semaphores[id].valid){
        num++;
        id=(id+1)%NUM_MAX_TASK;
        if(num==NUM_MAX_TASK) 
            return -1;
    }
    whole_semaphores[id].valid = 1;
    whole_semaphores[id].sem = val;
    return id;
}
