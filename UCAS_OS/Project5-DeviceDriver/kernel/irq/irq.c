#include <os/irq.h>
#include <os/time.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/stdio.h>
#include <assert.h>
#include <sbi.h>
#include <screen.h>
#include <csr.h>
#include <os/mm.h>
#include <pgtable.h>
#include <atomic.h>
#include <os/smp.h>
#include <os/lock.h>
#include <emacps/xemacps_example.h>
#include <plic.h>

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];
uintptr_t riscv_dtb;

extern list_head recv_queue;
extern list_head send_queue;
extern s32 FramesRx;
extern u64 bd_space[33];

void reset_irq_timer()
{
    // TODO clock interrupt handler.
    // TODO: call following functions when task4
    screen_reflush();
    timer_check();

    // note: use sbi_set_timer
    // remember to reschedule
    sbi_set_timer(get_ticks() + time_base/500);
    do_scheduler();
}

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    // TODO interrupt handler.
    // call corresponding handler by the value of `cause`
    if(cause & SCAUSE_IRQ_FLAG){
       irq_table[regs->scause & 0x7fffffffffffffff](regs,stval,cause);
    }else{ 
       exc_table[regs->scause](regs,stval,cause);
    }
  
}

void handle_int(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
    reset_irq_timer();
}

void handle_irq(regs_context_t *regs, int irq)
{
    // TODO: 
    // handle external irq from network device
    // let PLIC know that handle_irq has been finished
    if (!is_empty(&recv_queue)){
        uint32_t isr = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress, XEMACPS_ISR_OFFSET);
        XEmacPs_WriteReg(EmacPsInstance.Config.BaseAddress, XEMACPS_ISR_OFFSET, isr);
        if (isr & XEMACPS_IXR_FRAMERX_MASK){
            int current_Rxframe = 0;
            while (bd_space[current_Rxframe] & XEMACPS_RXBUF_NEW_MASK){
                current_Rxframe++;
            }
            if ((current_Rxframe>0) && (bd_space[current_Rxframe - 1] & XEMACPS_RXBUF_WRAP_MASK)){
                do_unblock(recv_queue.next);
            }
            plic_irq_eoi(irq);
        }
    }

    if (!is_empty(&send_queue)){
        uint32_t isr = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress, XEMACPS_ISR_OFFSET);
        XEmacPs_WriteReg(EmacPsInstance.Config.BaseAddress, XEMACPS_ISR_OFFSET, isr);
        if (isr & XEMACPS_IXR_TXCOMPL_MASK){ 
            uint32_t txsr = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress, XEMACPS_TXSR_OFFSET);
            if (txsr & XEMACPS_TXSR_TXCOMPL_MASK){
                XEmacPs_WriteReg(EmacPsInstance.Config.BaseAddress, XEMACPS_TXSR_OFFSET, txsr | XEMACPS_TXSR_TXCOMPL_MASK);
                do_unblock(send_queue.next);
            }
        }
    }

    Xil_DCacheFlushRange(0, 64);
    plic_irq_eoi(irq);

}

void init_exception()
{
    /* TODO: initialize irq_table and exc_table */
    /* note: handle_int, handle_syscall, handle_other, etc.*/
    for(int i=0;i<IRQC_COUNT;i++){
        irq_table[i] = handle_other;
    }
    irq_table[IRQC_S_TIMER] = handle_int;
    irq_table[IRQC_S_SOFT] = clear_ipi;
    irq_table[IRQC_S_EXT] = plic_handle_irq;
    for (int i=0;i<EXCC_COUNT;i++)
    {
        exc_table[i] = handle_other;
    }
    exc_table[EXCC_SYSCALL] = handle_syscall;
    exc_table[EXCC_INST_PAGE_FAULT ] = handle_pagefault;
    exc_table[EXCC_LOAD_PAGE_FAULT ] = handle_pagefault;
    exc_table[EXCC_STORE_PAGE_FAULT] = handle_pagefault;
    setup_exception();
}

void handle_other(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    char* reg_name[] = {
        "zero "," ra  "," sp  "," gp  "," tp  ",
        " t0  "," t1  "," t2  ","s0/fp"," s1  ",
        " a0  "," a1  "," a2  "," a3  "," a4  ",
        " a5  "," a6  "," a7  "," s2  "," s3  ",
        " s4  "," s5  "," s6  "," s7  "," s8  ",
        " s9  "," s10 "," s11 "," t3  "," t4  ",
        " t5  "," t6  "
    };
    for (int i = 0; i < 32; i += 3) {
        for (int j = 0; j < 3 && i + j < 32; ++j) {
            printk("%s : %016lx ",reg_name[i+j], regs->regs[i+j]);
        }
        printk("\n\r");
    }
    printk("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lx\n\r",
           regs->sstatus, regs->sbadaddr, regs->scause);
    printk("stval: 0x%lx cause: %lx\n\r",
           stval, cause);
    printk("sepc: 0x%lx\n\r", regs->sepc);
     printk("mhartid: 0x%lx\n\r", get_current_cpu_id());

    uintptr_t fp = regs->regs[8], sp = regs->regs[2];
    printk("[Backtrace]\n\r");
    printk("  addr: %lx sp: %lx fp: %lx\n\r", regs->regs[1] - 4, sp, fp);
    // while (fp < USER_STACK_ADDR && fp > USER_STACK_ADDR - PAGE_SIZE) {
    while (fp > 0x10000) {
        uintptr_t prev_ra = *(uintptr_t*)(fp-8);
        uintptr_t prev_fp = *(uintptr_t*)(fp-16);

        printk("  addr: %lx sp: %lx fp: %lx\n\r", prev_ra - 4, fp, prev_fp);

        fp = prev_fp;
    }

    assert(0);
}

void handle_pagefault(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    uintptr_t kva;
    kva = alloc_page_helper(stval, current_running[cpu_id]->pgdir,1);
    
}

void disk2mem(int block, int num, uintptr_t pa, uintptr_t va)
{
    sbi_sd_read(pa, num, block);
    prints("[swap] from SD block %d to MEM 0x%lx (v0x%lx) (#blocks %d)\n",
            block, pa, va, num);
    screen_reflush();
}

void mem2disk(int block, int num, uintptr_t pa, uintptr_t va)
{
    sbi_sd_write(pa, num, block);
    prints("[swap] from MEM 0x%lx (v0x%lx) to SD block %d (#blocks %d)\n",
            pa, va, block, num);
    screen_reflush();
}

/* void check_send_recv()
{
    if (!is_empty(&send_queue)){
        u32 txsr = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress, XEMACPS_TXSR_OFFSET);
        if ((txsr & XEMACPS_TXSR_TXCOMPL_MASK))
            do_unblock(send_queue.next);
    }
    if (!is_empty(&recv_queue)){
        if ((bd_space[FramesRx] & XEMACPS_RXBUF_NEW_MASK)){
            FramesRx++;
            do_unblock(recv_queue.next);
        }
    }
        
} */