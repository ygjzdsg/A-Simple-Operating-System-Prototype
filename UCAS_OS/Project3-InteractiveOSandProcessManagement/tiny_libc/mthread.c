#include <stdatomic.h>
#include <mthread.h>
#include <sys/syscall.h>
//typedef int mthread_mutex_t;


int mthread_mutex_init(void* handle)
{
    /* TODO: */
    invoke_syscall(MTHREAD_MUTEX_INIT,handle,IGNORE,IGNORE);
    return 0;
}

int mthread_mutex_lock(void* handle) 
{
    /* TODO: */
    return invoke_syscall(MTHREAD_MUTEX_LOCK,handle,IGNORE,IGNORE);;
}

int mthread_mutex_unlock(void* handle)
{
    /* TODO: */
    return invoke_syscall(MTHREAD_MUTEX_UNLOCK,handle,IGNORE,IGNORE);
}

int mthread_barrier_init(void* handle, unsigned count)
{
    // TODO:
    return invoke_syscall(MTHREAD_BARRIER_INIT,handle,count,IGNORE);

}
int mthread_barrier_wait(void* handle)
{
    // TODO:
    return invoke_syscall(MTHREAD_BARRIER_WAIT,handle,IGNORE,IGNORE);

}
int mthread_barrier_destroy(void* handle)
{
    // TODO:
    return invoke_syscall(MTHREAD_BARRIER_DESTROY,handle,IGNORE,IGNORE);
}

int mthread_semaphore_init(void* handle, int val)
{
    // TODO:
    return invoke_syscall(MTHREAD_SEMAPHORE_INIT,handle,val,IGNORE);
}
int mthread_semaphore_up(void* handle)
{
    // TODO:
    return invoke_syscall(MTHREAD_SEMAPHORE_UP,handle,IGNORE,IGNORE);
}
int mthread_semaphore_down(void* handle)
{
    // TODO:
    return invoke_syscall(MTHREAD_SEMAPHORE_DOWN,handle,IGNORE,IGNORE);
}
int mthread_semaphore_destroy(void* handle)
{
    // TODO:
    return invoke_syscall(MTHREAD_SEMAPHORE_DESTROY,handle,IGNORE,IGNORE);
}
