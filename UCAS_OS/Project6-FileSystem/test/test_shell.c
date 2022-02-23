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

#define MAXCHAR 200

#define SHELL_BEGIN 15
char path[100] = "/";

char uart_getchar(){
    char c;
    int i;
    while((i=sys_get_char())==-1){
        ;
    }
    c=(char)i;
    return c;
}
static void path_ret()
{
    int len = strlen(path);
    int i;
    for (i = len; i > 0; i--)
        if (path[i] != '/')
            path[i] = 0;
        else
            break;
        if (i)
            path[i] = 0;
}

static void path_change(char *name)
{
    int len = strlen(name);
    int i;
    if (name[0] == '/')
    {
        strcpy(path, name);
        return ;
    }
    for (i = 0; i < len; i++)
        if (name[i] == '/')
            break;
    if (i == len)
    {
        if (!strcmp(name,"."))
        {
        }
        else if(!strcmp(name, ".."))
        {
            path_ret();
        }
        else
        {
            if (strcmp(path, "/"))
            strcat(path, "/");
            strcat(path, name);
        }
    }else{
        char cur_name[16];
        char nxt_name[16];
        memset(cur_name, 0, 16);
        memset(nxt_name, 0, 16);
        memcpy(cur_name, name, i);
        memcpy(nxt_name, &name[i + 1], len - i - 1);
        if (!strcmp(cur_name,"."))
        {
        }
        else if(!strcmp(cur_name, ".."))
        {
            path_ret();
        }
        else
        {
            if (strcmp(path, "/"))
            strcat(path, "/");
            strcat(path, cur_name);
        }
        path_change(nxt_name);
    }
}

void command(char buf[]){
    char command_part1[MAXCHAR];
    char temp[MAXCHAR];
    char temp0[MAXCHAR];
    char temp1[MAXCHAR];
    char temp2[MAXCHAR];
    char temp3[MAXCHAR];
    char temp4[MAXCHAR];
    char directory[MAXCHAR];
 
    int i,j,k,m,n,l;
    int mask,pid;
    int argc=0;

    char* argv[5];
    argv[1]=temp1;
    argv[2]=temp2;
    argv[3]=temp3;
    argv[4]=temp4;
    argv[0]=temp0;

    i=j=k=l=m=n=0;
    int test_num=0;
    while(buf[i]!='\0' && buf[i]!=' '){
        command_part1[j]=buf[i];
        i++;
        j++;
    }
    command_part1[j]='\0';

    while(buf[i]==' '){
        i++;
        l=0;
        while((buf[i]!='\0') && (buf[i]!=' ') ){
            argv[argc][l]=buf[i];
            if(argc==0){
                test_num = test_num*10 + (buf[i]-'0');
            }
            i++;
            l++;
        }
        argv[argc][l]='\0';
        argc++;
    }

    if(!strcmp(command_part1,"exec")){
        pid_t pid;
        pid = sys_exec(argv[0],argc,argv,AUTO_CLEANUP_ON_EXIT);
        printf("exec process[%d]:%s.\n",pid,argv[0]);
        return;
    }else if(!strcmp(command_part1,"mkfs")){
        sys_mkfs();
    }else if(!strcmp(command_part1,"ls")){
        if(argc==0){
            directory[0] = '.';
            directory[1] = '\0';
            sys_ls(directory);
        }else if(argc==1){
            if(!strcmp(argv[0], "-l")){
                directory[0] = '.';
                directory[1] = '\0';
                sys_ls_l(directory);
            }else{
                strcpy(directory, argv[0]);
                sys_ls(directory);
            }
        }else{
            if(!strcmp(argv[0], "-l")){
                strcpy(directory, argv[1]);
                sys_ls_l(directory);
            }
        }
    }else if(!strcmp(command_part1,"ln")){
        sys_ln(argv[0], argv[1]);
    }else if(!strcmp(command_part1,"statfs")){
        sys_statfs();
    }else if(!strcmp(command_part1,"mkdir")){
        sys_mkdir(argv[0]);
    }else if(!strcmp(command_part1,"rmdir")){
        sys_rmdir(argv[0]);
    }else if(!strcmp(command_part1,"rm")){
        sys_rm(argv[0]);
    }else if(!strcmp(command_part1,"cd")){
        sys_cd(argv[0]);
    }else if(!strcmp(command_part1,"touch")){
        sys_touch(argv[0]);
    }else if(!strcmp(command_part1,"cat")){
        sys_cat(argv[0]);
    }else if(!strcmp(command_part1,"ts")){
        sys_show_exec();
    }else if(!strcmp(command_part1,"ps")){
        sys_process_show();
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


void main()
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
