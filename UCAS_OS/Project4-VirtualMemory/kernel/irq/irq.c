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

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];
uintptr_t riscv_dtb;
int bid = 10;


//LIST_HEAD(used_page_queue);

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

void init_exception()
{
    /* TODO: initialize irq_table and exc_table */
    /* note: handle_int, handle_syscall, handle_other, etc.*/
    for(int i=0;i<IRQC_COUNT;i++){
        irq_table[i] = handle_other;
    }
    irq_table[IRQC_S_TIMER] = handle_int;
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
    list_node_t *check_page_list;
    page_t *check_page;
    page_t *exchange_page;
    list_node_t *exchange_page_list;
    check_page_list=current_running[cpu_id]->used_page_queue.next;
    uintptr_t * level_0;

    while(check_page_list!=&current_running[cpu_id]->used_page_queue){
        
        check_page = list_entry(check_page_list,page_t,list);
        if(check_page->va==((stval >> NORMAL_PAGE_SHIFT) << NORMAL_PAGE_SHIFT)){
            if(current_running[cpu_id]->pagenum==4){
                exchange_page_list=current_running[cpu_id]->page_list.next;
                exchange_page=list_entry(exchange_page_list,page_t,list);
                
                exchange_page->block=bid;
                mem2disk(bid, 8, exchange_page->pa, exchange_page->va);
                bid=bid +8;
                level_0 = check_page_helper(exchange_page->va,current_running[cpu_id]->pgdir);
        
                (*level_0) &= ~((uint64_t)( _PAGE_ACCESSED | _PAGE_PRESENT   | _PAGE_DIRTY));

/*                 set_attribute(level_0, _PAGE_READ     | _PAGE_WRITE    | _PAGE_EXEC 
                        | _PAGE_ACCESSED  | _PAGE_PRESENT   | _PAGE_DIRTY  
                       |  _PAGE_USER ); */
                delete_element(&exchange_page->list);
                add_element(&exchange_page->list,&current_running[cpu_id]->used_page_queue);
    
                disk2mem(check_page->block, 8, check_page->pa, check_page->va);
                check_page->block=0;

                level_0 = check_page_helper(check_page->va,current_running[cpu_id]->pgdir);
                (*level_0) |= ((uint64_t)( _PAGE_ACCESSED | _PAGE_PRESENT   | _PAGE_DIRTY));

                delete_element(&check_page->list);
                add_element(&check_page->list,&current_running[cpu_id]->page_list);
                
            }else{
                current_running[cpu_id]->pagenum++;
                disk2mem(check_page->block, 8, check_page->pa, check_page->va);
                check_page->block=0;

                level_0 = check_page_helper(check_page->va,current_running[cpu_id]->pgdir);
                (*level_0) |= ((uint64_t)( _PAGE_ACCESSED | _PAGE_PRESENT   | _PAGE_DIRTY));
                
                delete_element(&check_page->list);
                add_element(&check_page->list,&current_running[cpu_id]->page_list);
            }
            local_flush_tlb_all();
            return;
        }
        check_page_list=check_page_list->next;    
        
    }

    if(current_running[cpu_id]->pagenum==4){

        exchange_page_list=current_running[cpu_id]->page_list.next;
        exchange_page=list_entry(exchange_page_list,page_t,list);
        
        exchange_page->block=bid;
        mem2disk(bid, 8, exchange_page->pa, exchange_page->va);
        bid=bid +8;

        level_0 = check_page_helper(exchange_page->va,current_running[cpu_id]->pgdir);
        (*level_0) &= ~((uint64_t)( _PAGE_ACCESSED | _PAGE_PRESENT   | _PAGE_DIRTY));

        delete_element(&exchange_page->list);
        add_element(&exchange_page->list,&current_running[cpu_id]->used_page_queue);


        kva=alloc_page_helper(stval, current_running[cpu_id]->pgdir,1);
        page_t *new_page = (page_t *)kmalloc(sizeof(page_t));
        new_page->block=0;
        new_page->pa=kva2pa(kva);
        new_page->va = ((stval >> NORMAL_PAGE_SHIFT) << NORMAL_PAGE_SHIFT);
        add_element(&new_page->list,&current_running[cpu_id]->page_list);

    }else{
        current_running[cpu_id]->pagenum++;
        kva=alloc_page_helper(stval, current_running[cpu_id]->pgdir,1);
        page_t *new_page = (page_t *)kmalloc(sizeof(page_t));
        new_page->block=0;
        new_page->pa=kva2pa(kva);
        new_page->va = ((stval >> NORMAL_PAGE_SHIFT) << NORMAL_PAGE_SHIFT);
        add_element(&new_page->list,&current_running[cpu_id]->page_list);
        //-----------
        exchange_page_list=current_running[cpu_id]->page_list.next;
        exchange_page=list_entry(exchange_page_list,page_t,list);

    }
    local_flush_tlb_all();
    return;
    
}

void disk2mem(int block, int num, uintptr_t pa, uintptr_t va)
{
    sbi_sd_read(pa, num, block);
    prints("[swap] from SD block %d to MEM 0x%lx (v0x%lx) (#blocks %d)\n\r",
            block, pa, va, num);
    screen_reflush();
}

void mem2disk(int block, int num, uintptr_t pa, uintptr_t va)
{
    sbi_sd_write(pa, num, block);
    prints("[swap] from MEM 0x%lx (v0x%lx) to SD block %d (#blocks %d)\n\r",
            pa, va, block, num);
    screen_reflush();
}
