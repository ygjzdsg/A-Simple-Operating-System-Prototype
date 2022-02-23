#include <os/ioremap.h>
#include <os/mm.h>
#include <pgtable.h>
#include <type.h>

// maybe you can map it to IO_ADDR_START ?
static uintptr_t io_base = IO_ADDR_START;

void *ioremap(unsigned long phys_addr, unsigned long size)
{
    // map phys_addr to a virtual address
    // then return the virtual address
    uintptr_t pgdir = 0xffffffc05e000000;   
    uint64_t vaddr = io_base;
    uint64_t va,vpn0,vpn1,vpn2;
    while (size != 0){
        va = io_base;
        vpn0 = (va >> 12) & ~(~0 << 9);
        vpn1 = (va >> 21) & ~(~0 << 9);
        vpn2 = (va >> 30) & ~(~0 << 9);
        PTE *level2 = (PTE *)pgdir + vpn2;
        PTE *level1 = NULL;
        PTE *pte = NULL;
        if (((*level2) & 0x1) == 0){
            ptr_t newpage = allocPage();
            *level2 = (kva2pa(newpage) >> 12) << 10;
            set_attribute(level2, _PAGE_PRESENT);
            level1 = (PTE *)newpage + vpn1;
        }else{
            level1 = (PTE *)pa2kva(((*level2) >> 10) << 12) + vpn1;
        }
        
        if (((*level1) & 0x1) == 0){
            ptr_t newpage = allocPage();
            *level1 = (kva2pa(newpage) >> 12) << 10;
            set_attribute(level1, _PAGE_PRESENT);
            pte = (PTE *)newpage + vpn0;
        }else{
            pte = (PTE *)pa2kva(((*level1) >> 10) << 12) + vpn0;
        }

        set_pfn(pte, phys_addr >> 12);
  
        set_attribute(pte, (_PAGE_READ     | _PAGE_WRITE    | _PAGE_EXEC 
                           | _PAGE_ACCESSED | _PAGE_PRESENT  | _PAGE_DIRTY));

        io_base = (uint64_t)io_base + PAGE_SIZE;
        phys_addr = (uint64_t)phys_addr + PAGE_SIZE;
        size -= PAGE_SIZE;
    }
    local_flush_tlb_all();
    return vaddr;
}

void iounmap(void *io_addr)
{
    // TODO: a very naive iounmap() is OK
    // maybe no one would call this function?
    // *pte = 0;
}
