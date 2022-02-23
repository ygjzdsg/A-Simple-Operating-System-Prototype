#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

#include <os.h>

static char buff[64];

int main(void)
{
    int i, j;
    int fd = sys_fopen("1.txt", O_RDWR);

    sys_move_cursor(1,1);
    printf("Start to write hello on 8M\n");
    sys_sleep(1);
    sys_lseek(fd,8*1024*1024,0);
    for (i = 0; i < 10; i++)
    {
        sys_fwrite(fd, "hello world!\n", 13);
    }

    printf("Start to read hello from 8M\n");
    sys_sleep(1);
    for (i = 0; i < 10; i++)
    {
        sys_fread(fd, buff, 13);
        for (j = 0; j < 13; j++)
        {
            printf("%c", buff[j]);
        }
    }
    
    sys_fclose(fd);
}