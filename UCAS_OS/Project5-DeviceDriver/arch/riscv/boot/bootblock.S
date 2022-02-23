#include <asm/sbidef.h>
#include <asm/sbiasm.h>
#include <csr.h>

// 1. kernel address (move kernel to here ~)
.equ kernel, 0x50301000

// 2. kernel main address (jmp here to start kernel main!)
.equ kernel_main, 0x50301000

// 3. os size location (get the size of your OS here)
.equ os_size_loc, 0x502001fc

.text
.global main

main:
	// fence on all memory and I/O
	fence

	// a0 is mhartid, a1 is the pointer of fdt
	move s0,a0
	move s1,a1

	// output a \n
	li a0, 10
	SBI_CALL SBI_CONSOLE_PUTCHAR

	// The core whose mhartid=0 is selected to boot the kernel.
	// Other cores should wait until the kernel's initialization
	// has been finished.
 	move a0, s0
	bnez a0, secondary

	// your code
	// 1) task1 call BIOS print string "It's bootblock!"
	la a0, msg
	SBI_CALL SBI_CONSOLE_PUTSTR

	// 2) task2 call BIOS read kernel in SD card and jump to kernel start
    //	lui s0, %hi(os_size_loc)
    //    addi s0, s0, %lo(os_size_loc)
    la s0, os_size_loc

    lh s0, 0(s0)
    li s1, 1
    li s2, 32768
    la s3, kernel
copy:
    mv a0, s3
    li a1, 64
    mv a2, s1
    SBI_CALL SBI_SD_READ
    addi s1, s1, 64
    add  s3, s3, s2
    addi s0, s0, -64
    bgtz s0, copy	

    fence.i 
	la t0, kernel_main
	jr t0 

secondary:
	/* TODO: 
	 * 1. Mask all interrupts
	 * 2. let stvec pointer to kernel_main
	 * 3. enable software interrupt for ipi
	 */
    fence
    csrw CSR_SIE, zero
    csrw CSR_SIP, zero

    // Set stvec to kernel entry
    la t0, kernel_main
    csrw CSR_STVEC, t0
    
    // Enable supervisor software interrupt (ipi)
    li t0, SIE_SSIE
    csrw CSR_SIE, t0
    
    li t0, SR_SIE
    csrw CSR_SSTATUS, t0

    fence.i
wait_for_wakeup:
	wfi
	j wait_for_wakeup

// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string  "It's a bootloader...\n\r"
