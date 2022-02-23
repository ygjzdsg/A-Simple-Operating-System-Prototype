#include <stdio.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <time.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    long mem2 = 0;
	uintptr_t mem1 = 0;
    mem1 = 0x10008000;
    sys_move_cursor(1, 1);
	for (int i = 0; i < 5; i++)
	{
		mem1=mem1+i*4096;
		mem2 = rand();
		*(long*)mem1 = mem2;

		if (*(long*)mem1 != mem2) {
			printf("0x%lx, %ld Error\n", mem1, mem2);
            return 0;
		}else{
            printf("0x%lx, %ld Success\n", mem1, mem2);
        }
        sys_sleep(1);
	}
    mem1 = 0x10008000;
    for (int i = 0; i < 5; i++)
	{
		mem1=mem1+i*4096;
		mem2 = rand();
		*(long*)mem1 = mem2;

		if (*(long*)mem1 != mem2) {
			printf("0x%lx, %ld Error\n", mem1, mem2);
            return 0;
		}else{
            printf("0x%lx, %ld Success\n", mem1, mem2);
        }
        sys_sleep(1);
	}

	printf("All Success!\n");
	return 0;
}