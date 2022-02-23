#include <os/mm.h>
#include <pgtable.h>

ptr_t memCurr = FREEMEM;

ptr_t freePageList = (ptr_t)&freePageList;

ptr_t allocPage()
{
    memCurr += PAGE_SIZE;
    return memCurr;
}

void freePage(ptr_t baseAddr)
{
    
}

void* kmalloc(size_t size)
{
    ptr_t ret = ROUND(memCurr, 4);
    memCurr = ret + size;
    return (void*)ret;
}
uintptr_t shm_page_get(int key)
{
    // TODO(c-core):
}

void shm_page_dt(uintptr_t addr)
{
    // TODO(c-core):
}

/* this is used for mapping kernel virtual address into user page table */
void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir)
{
    // TODO:
    uint32_t len = 0x800;
    kmemcpy((uint8_t *)(dest_pgdir+0x800), (uint8_t *)(src_pgdir+0x800), len);
}

/* allocate physical page for `va`, mapping it into `pgdir`,
   return the kernel virtual address for the page.
   */
uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir,int mode)
{
    uint64_t vpn0=(va >> 12) & ~(~0 << 9);
    uint64_t vpn1=(va >> 21) & ~(~0 << 9);
    uint64_t vpn2=(va >> 30) & ~(~0 << 9);
    // Find PTE
    PTE *level_2 = (PTE *)pgdir + vpn2;
    PTE *level_1 = NULL;
    PTE *level_0 = NULL;

    if (((*level_2) & 0x1) == 0){
        ptr_t newpage = allocPage();
        *level_2 = (kva2pa(newpage) >> 12) << 10;
        set_attribute(level_2, (mode ? _PAGE_USER : 0) | _PAGE_PRESENT| _PAGE_ACCESSED | _PAGE_DIRTY);
        level_1 = (PTE *)newpage + vpn1;
    }else{
        level_1 = (PTE *)pa2kva(((*level_2) >> 10) << 12) + vpn1;
    }

    if (((*level_1) & 0x1) == 0){
        ptr_t newpage = allocPage();
        *level_1 = (kva2pa(newpage) >> 12) << 10;
        set_attribute(level_1, (mode ? _PAGE_USER : 0) | _PAGE_PRESENT| _PAGE_ACCESSED | _PAGE_DIRTY);
        level_0 = (PTE *)newpage + vpn0;
    }else{
        level_0 = (PTE *)pa2kva(((*level_1) >> 10) << 12) + vpn0;
    }

    if ((*level_0 & 0x1) != 0)
        return 0;

    ptr_t newpage = allocPage();
    set_pfn(level_0, (uint64_t)kva2pa(newpage) >> 12);

    uint64_t pte_flags = _PAGE_READ     | _PAGE_WRITE    | _PAGE_EXEC 
                       | _PAGE_ACCESSED | _PAGE_PRESENT  | _PAGE_DIRTY 
                       | (mode == 1 ? _PAGE_USER : 0);
    set_attribute(level_0, pte_flags);
    return (newpage >> 12) << 12;    
}

uintptr_t check_page_helper(uintptr_t va, uintptr_t pgdir)
{
    uint64_t vpn[] = {(va >> 12) & ~(~0 << 9),
                      (va >> 21) & ~(~0 << 9),
                      (va >> 30) & ~(~0 << 9)};
    // Find PTE
    PTE *level_2 = (PTE *)pgdir + vpn[2];
    if (((*level_2) & 0x1) == 0)
        return 0;
    PTE *level_1 = (PTE *)pa2kva(((*level_2) >> 10) << 12) + vpn[1];
    if (((*level_1) & 0x1) == 0)
        return 0;
    PTE *final_pte = (PTE *)pa2kva(((*level_1) >> 10) << 12) + vpn[0];
    if (((*final_pte) & 0x1) == 0)
        return 0;
    return final_pte;
}

