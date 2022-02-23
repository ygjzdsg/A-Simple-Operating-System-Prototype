#include <sys/syscall.h>
#include <stdint.h>
#include <mthread.h>

void sys_sleep(uint32_t time)
{
    invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE, IGNORE);
}

void sys_write(char *buff)
{
    invoke_syscall(SYSCALL_WRITE, (uintptr_t)buff, IGNORE, IGNORE, IGNORE);
}

void sys_reflush()
{
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{
    invoke_syscall(SYSCALL_CURSOR, x, y, IGNORE, IGNORE);
    
}


long sys_get_timebase()
{
    return invoke_syscall(SYSCALL_GET_TIMEBASE, IGNORE, IGNORE, IGNORE, IGNORE);
}

long sys_get_tick()
{
   return invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE, IGNORE); 
}


void sys_yield()
{
    // TODO:
     invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE, IGNORE);
    //   or
    //do_scheduler();
    // ???
}

int sys_get_char(){
    return invoke_syscall(SYSCALL_GET_CHAR, IGNORE, IGNORE, IGNORE, IGNORE);
}


pid_t sys_spawn(task_info_t *info, void* arg, spawn_mode_t mode){
    return invoke_syscall(SYSCALL_SPAWN, (uintptr_t)info, (uintptr_t)arg, mode, IGNORE);
}

void sys_exit(void){
    invoke_syscall(SYSCALL_EXIT, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_kill(pid_t pid){
    return invoke_syscall(SYSCALL_KILL, pid, IGNORE, IGNORE, IGNORE);
}

int sys_waitpid(pid_t pid){
    return invoke_syscall(SYSCALL_WAITPID, pid, IGNORE, IGNORE, IGNORE);
}

void sys_process_show(void){
    invoke_syscall(SYSCALL_PS, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_screen_clear(void){
    invoke_syscall(SYSCALL_SCREEN_CLEAR, IGNORE, IGNORE, IGNORE, IGNORE);
}

pid_t sys_getpid(){
    return invoke_syscall(SYSCALL_GETPID, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_taskset_p(int mask, int pid){
    invoke_syscall(SYSCALL_TASKSET_P, mask, pid, IGNORE, IGNORE);
}

void sys_taskset(int mask, task_info_t *info, spawn_mode_t mode){
    invoke_syscall(SYSCALL_TASKSET, mask, (uintptr_t)info, mode, IGNORE);
}

pid_t sys_exec(const char *file_name, int argc, char* argv[], spawn_mode_t mode){
        return invoke_syscall(SYSCALL_EXEC, (uintptr_t)file_name, argc, (uintptr_t)argv, mode);
}

void sys_show_exec(){
        invoke_syscall(SYSCALL_SHOW_EXEC, IGNORE, IGNORE, IGNORE, IGNORE);
}

int binsemget(int key){
    return invoke_syscall(SYSCALL_BINSEMGET, key, IGNORE, IGNORE,IGNORE);
}

int binsemop(int binsem_id, int op){
    return invoke_syscall(SYSCALL_BINSEMOP, binsem_id, op, IGNORE,IGNORE);
}

int mthread_create(mthread_t *thread, void (*start_routine)(void*), void *arg){
    return invoke_syscall(MTHREAD_CREATE, thread, start_routine, arg ,IGNORE);
}

int mthread_join(mthread_t thread){
    ;
}



void sys_mkfs()
{
    invoke_syscall(SYSCALL_MKFS, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_statfs()
{
    invoke_syscall(SYSCALL_STATFS, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_cd(char *dir_name)
{
    invoke_syscall(SYSCALL_CD, dir_name, IGNORE, IGNORE, IGNORE);
}

void sys_mkdir(char *dir_name)
{
    invoke_syscall(SYSCALL_MKDIR, dir_name, IGNORE, IGNORE, IGNORE);
}

void sys_rmdir(char *dir_name)
{
    invoke_syscall(SYSCALL_RMDIR, dir_name, IGNORE, IGNORE, IGNORE);
}

void sys_ls(char *dir_name)
{
    invoke_syscall(SYSCALL_LS, dir_name, IGNORE, IGNORE, IGNORE);
}

void sys_ls_l(char *dir_name)
{
    invoke_syscall(SYSCALL_LS_L, dir_name, IGNORE, IGNORE, IGNORE);
}

void sys_touch(char *file_name)
{
    invoke_syscall(SYSCALL_TOUCH, file_name, IGNORE, IGNORE, IGNORE);
}

void sys_cat(char *file_name)
{
    invoke_syscall(SYSCALL_CAT, file_name, IGNORE, IGNORE, IGNORE);
}

int sys_fopen(char *name, int access)
{
    return invoke_syscall(SYSCALL_FOPEN, name, access, IGNORE, IGNORE);
}

void sys_fread(int fd, char *buff, int size)
{
    invoke_syscall(SYSCALL_FREAD, fd, buff, size, IGNORE);
}

void sys_fwrite(int fd, char *buff, int size)
{
    invoke_syscall(SYSCALL_FWRITE, fd, buff, size, IGNORE);
}

void sys_lseek(int fd, int offset, int whence)
{
    invoke_syscall(SYSCALL_LSEEK, fd, offset, whence, IGNORE);
}

void sys_fclose(int fd)
{
    invoke_syscall(SYSCALL_FCLOSE, fd, IGNORE, IGNORE, IGNORE);
}

void sys_ln(char *source, char *link_name)
{
    invoke_syscall(SYSCALL_LN, source, link_name, IGNORE, IGNORE);
}

void sys_rm(char *file_name)
{
    invoke_syscall(SYSCALL_RM, file_name, IGNORE, IGNORE, IGNORE);
}