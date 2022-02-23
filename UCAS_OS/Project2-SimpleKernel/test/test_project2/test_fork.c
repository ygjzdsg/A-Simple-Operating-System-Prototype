#include <test2.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/syscall.h>

void fork_task(void){
    int print_location = 1;
    int flag=0;
    char c;
    int priority = 1;
    int child = 1;

    for (int i=0;;i++){
        sys_move_cursor(1,print_location);
        if (flag==1){
            printf("> [TASK] This is child process (priority %d) (%d)\n", priority, i);
        }else{
            printf("> [TASK] This is father process (priority %d) (%d)\n", priority, i);
            c=sys_read();
            if(c>'0' && c<'9'){
               if(sys_fork()==0){
                  flag++;
                  print_location += child;
                  priority=c-'0';
                  sys_set_priority(priority);
               }else{
                  child++;
               }
            }
        }
    }
}