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
