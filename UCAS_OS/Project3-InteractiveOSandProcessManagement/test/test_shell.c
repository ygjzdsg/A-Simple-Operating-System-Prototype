/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                  The shell acts as a task running in user mode.
 *       The main function is to make system calls through the user's output.
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

#include <test.h>
#include <string.h>
#include <os.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>

#define MAXCHAR 20

struct task_info task_test_waitpid = {
    (uintptr_t)&wait_exit_task, USER_PROCESS};
struct task_info task_test_semaphore = {
    (uintptr_t)&semaphore_add_task1, USER_PROCESS};
struct task_info task_test_barrier = {
    (uintptr_t)&test_barrier, USER_PROCESS};

struct task_info strserver_task = {(uintptr_t)&strServer, USER_PROCESS};
struct task_info strgenerator_task = {(uintptr_t)&strGenerator, USER_PROCESS};

struct task_info task_test_multicore = {(uintptr_t)&test_multicore, USER_PROCESS};
struct task_info task_test_affinity = {(uintptr_t)&test_affinity, USER_PROCESS};

struct task_info multimail_task = {(uintptr_t)&multimail, USER_PROCESS};

static struct task_info *test_tasks[16] = {&task_test_waitpid,
                                           &task_test_semaphore,
                                           &task_test_barrier,
                                           &task_test_multicore,
                                           &strserver_task, &strgenerator_task,
                                           &task_test_affinity,
                                           &multimail_task};
static int num_test_tasks = 8;

#define SHELL_BEGIN 15

char uart_getchar(){
    char c;
    int i;
    while((i=sys_get_char())==-1){
        ;
    }
    c=(char)i;
    return c;
}

void command(char buf[]){
    char command_part1[MAXCHAR];
    char command_part2[MAXCHAR];
    char command_part3[MAXCHAR];
    char command_part4[MAXCHAR];
    int i,j,k,m,n;
    int mask,pid;
    i=j=k=m=n=0;
    int test_num=0;
    while(buf[i]!='\0' && buf[i]!=' '){
        command_part1[j]=buf[i];
        i++;
        j++;
    }
    command_part1[j]='\0';

    

    if(buf[i]==' '){
        i++;
        while((buf[i]!='\0') && (buf[i]!=' ') ){
            command_part2[k]=buf[i];
            test_num = test_num*10 + (buf[i]-'0');
            i++;
            k++;
        }
        command_part2[k]='\0';
    }

    if(buf[i]==' '){
        i++;
        while((buf[i]!='\0') && (buf[i]!=' ') ){
            command_part3[m]=buf[i];
            i++;
            m++;
        }
        command_part3[m]='\0';
    }

    if(buf[i]==' '){
        i++;
        while((buf[i]!='\0') && (buf[i]!=' ') ){
            command_part4[n]=buf[i];
            i++;
            n++;
        }
        command_part4[n]='\0';
    }

    if(!strcmp(command_part1,"taskset")){
       if(!strcmp(command_part2,"-p")){
           if(!strcmp(command_part3,"0x1")){
               mask=1;
           }else if(!strcmp(command_part3,"0x2")){
               mask=2;
           }else if(!strcmp(command_part3,"0x3")){
               mask=3;
           }
           for(int q=0;command_part4[q]!='\0';q++){
                pid = pid*10 + (command_part4[q]-'0');
           }
           sys_taskset_p(mask,pid);
           printf("%s %s 0x%d %d.\n",command_part1,command_part2,mask,pid);
       }else{
           if(!strcmp(command_part2,"0x1")){
               mask=1;
           }else if(!strcmp(command_part2,"0x2")){
               mask=2;
           }else if(!strcmp(command_part2,"0x3")){
               mask=3;
           }
           for(int q=0;command_part3[q]!='\0';q++){
                pid = pid*10 + (command_part3[q]-'0');
           }
           sys_taskset(mask,test_tasks[pid],AUTO_CLEANUP_ON_EXIT);
           printf("%s 0x%d %d.\n",command_part1,mask,pid);
       }
       return;
    }


    if(!strcmp(command_part1,"ps")){
        sys_process_show();
    }else if(!strcmp(command_part1,"exec")){
        pid_t pid;
        pid = sys_spawn(test_tasks[test_num], NULL, AUTO_CLEANUP_ON_EXIT);
        printf("exec process[%d] pid[%d].\n",test_num,pid);
    }else if(!strcmp(command_part1,"kill")){
        int killed;
        killed = sys_kill(test_num);
        if((killed)){
            printf("kill process[%d].\n",test_num);
        }else{
            printf("process[%d] is not found.\n",test_num);
        }
    }else if(!strcmp(command_part1,"exit")){
        sys_exit();
    }else if(!strcmp(command_part1,"clear")){
        sys_screen_clear();
        sys_move_cursor(1, SHELL_BEGIN);
        printf("------------------- COMMAND -------------------\n");
    }else if(!strcmp(command_part1,"wait")){
        int wait;
        wait = sys_waitpid(test_num);
        if(wait){
            printf("wait for process[%d].\n",test_num);
        }else{
            printf("process[%d] is not found.\n",test_num);
        }
    }else{
        printf("Unknown command\n");
    }
    return;

}
void test_shell()
{
    // TODO:
    sys_move_cursor(1, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
    printf("> root@UCAS_YB_OS: ");

    char buf[MAXCHAR];
    char c;
    int i=0;
    while (1)
    {
        // TODO: call syscall to read UART port
        c=uart_getchar();

        // TODO: parse input
        // note: backspace maybe 8('\b') or 127(delete)
        if(c=='\r' || c=='\n'){
            printf("\n");
            buf[i]='\0';
            // TODO: ps, exec, kill, clear
            command(buf);
            i=0;
            printf("> root@UCAS_YB_OS: ");
        }else if(c == 8 || c == 127){
            i--;
            printf("\b");
        }else{
            buf[i]=c;
            printf("%c",c);
            i++;
        }
        
    }
}
