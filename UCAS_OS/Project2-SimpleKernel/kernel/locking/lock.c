#include <os/lock.h>
#include <os/sched.h>
#include <atomic.h>

void do_mutex_lock_init(mutex_lock_t *lock)
{
    /* TODO */
    lock->lock.status=UNLOCKED;
    init_list(&lock->block_queue);
}

void do_mutex_lock_acquire(mutex_lock_t *lock)
{
    /* TODO */
  
    while (lock->lock.status == LOCKED){
        current_running->status=TASK_BLOCKED;
        do_block(&(current_running->list), &(lock->block_queue));
    }
        
    lock->lock.status = LOCKED;
        
}

void do_mutex_lock_release(mutex_lock_t *lock)
{
    /* TODO */
    while (!is_empty(&(lock->block_queue))){
        lock->lock.status=LOCKED;
        do_unblock(lock->block_queue.next);
        current_running->status=TASK_READY;
    }
    lock->lock.status = UNLOCKED;
        
        
}

mutex_lock_t locks[NUM_MAX_TASK];
int ids[NUM_MAX_TASK]={0};
int mutex_get(int key){
    int i = key%NUM_MAX_TASK;
    
    if(ids[i]==0){
        do_mutex_lock_init(&locks[i]);
        ids[i]=1;
    }
    
    return key%NUM_MAX_TASK;
}

int mutex_lock_func(int handle){
    do_mutex_lock_acquire(&locks[handle]);
}

int mutex_unlock(int handle){
    do_mutex_lock_release(&locks[handle]);
}

int mutex_op(int handle, int op){
    int i = mutex_get(handle);
    if(op==0){
        mutex_lock_func(i);
    }else{
        mutex_unlock(i);
    }
    return i;
} 