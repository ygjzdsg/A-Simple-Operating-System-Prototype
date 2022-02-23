#include <asm/sbidef.h>
#include <asm/sbiasm.h>

// #define BOOT_LOADER_SIG_OFFSET 0x1fe
// #define OS_SIZE_LOC (BOOT_LOADER_SIG_OFFSET - 2)
// #define OS_SIZE_LOC 0x1fc

.equ os_size_loc1, 0x502001fc
.equ os_size_loc2, 0x502001fa

// 1. kernel address (move kernel to here ~)
.equ kernel, 0x50200000

// 2. kernel main address (jmp here to start kernel main!)
.equ kernel_main, 0x50200000


.equ init_new_bootblock, 0x50201000
.equ bbl_en_offset, 0x50

.text
.global main

main:
	// fence on all memory and I/O
	fence


        
    lui t0, %hi(os_size_loc1)
    addi t0, t0, %lo(os_size_loc1)
	lh t0, (t0)
    lui t1, %hi(os_size_loc2)
    addi t1, t1, %lo(os_size_loc2)
	lh t1, (t1)
	bge t0,t1,new_bootblock
	addi t0,t1,0

new_bootblock:
	li t1, 0x200
	mul t1, t1, t0
	
    lui a0, %hi(init_new_bootblock)
    addi a0, a0, %lo(init_new_bootblock)
	add a0,a0,t1
    addi t1,a0,0  
    li a1, 1
    li a2, 0
	SBI_CALL SBI_SD_READ
	fence.i 
	addi t1, t1, bbl_en_offset
    jr t1
    
	//choose kernel  
    // print a \n to avoid system output error...
	li a0, 10
	SBI_CALL SBI_CONSOLE_PUTCHAR

	// 1) task1 call BIOS print string "It's bootblock!"
	la a0, msg
	SBI_CALL SBI_CONSOLE_PUTSTR
	
	//SD read kernel1  
	lui t0, %hi(os_size_loc1)
    addi t0, t0, %lo(os_size_loc1)
	lh t0, (t0)
    lui t1, %hi(os_size_loc2)
    addi t1, t1, %lo(os_size_loc2)
	lh t1, (t1)

    li t2,-1
	li t3,'2'
choice:
        SBI_CALL SBI_CONSOLE_GETCHAR
        beq a0,t2,choice
	beq a0,t3,kernel2

kernel1:
        addi a1,t0,0
	li a2, 1
	la t2, begin
        jr t2

kernel2:
        addi a1,t1,0
	addi a2,t0,1

begin:
       la a0, kernel
        SBI_CALL SBI_SD_READ
	fence.i
        
        la t1,kernel_main
	jr t1

// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n\rChoose kernel1 / kernel2\n\r"
