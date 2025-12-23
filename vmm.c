#include "vmm.h"
#include "pmm.h"
#include "terminal.h"

/* 页目录与页表是 4KB 对齐的数组，每个包含 1024 个 32位条目 */
/* 我们不静态定义，而是通过 PMM 动态请求物理页 */

extern void load_cr3(uint32_t page_directory_phys);
extern void enable_paging(void);

/* 内联汇编辅助函数，如果没在汇编里定义，就在这里写内联 */
static inline void set_cr3(uint32_t pde_phys) {
    asm volatile("mov %0, %%cr3" :: "r"(pde_phys));
}

static inline void enable_paging_bit(void) {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= PAGING_FLAG;
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
}

void vmm_init(void) {
    /* 1. 分配一个页目录 (Page Directory) */
    /* 注意：分配出来的地址是物理地址，目前还没有开启分页，所以可以直接用 */
    uint32_t pd_phys = pmm_alloc_page();
    uint32_t pt_phys = pmm_alloc_page();

    if (pd_phys == 0 || pt_phys == 0) {
        terminal_writestring("VMM Error: Failed to allocate PMM pages for PD/PT\n");
        return;
    }

    uint32_t* pd = (uint32_t*)pd_phys;
    uint32_t* pt = (uint32_t*)pt_phys;

    /* 2. 清空页目录 */
    for (int i = 0; i < 1024; i++) {
        pd[i] = 0x00000002; /* RW, Supervisor, Not Present */
    }

    /* 3. 填充第一个页表 (Identity Mapping 0-4MB) */
    /* 每一个 PTE 映射 4KB */
    /* 0x00000000 -> 0x00000000, 0x00001000 -> 0x00001000 ... */
    for (int i = 0; i < 1024; i++) {
        pt[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_RW;
    }

    /* 4. 填写页目录项 (Reference the Page Table) */

    /* [Identity Mapping] 映射低端 4MB (Virtual 0x00000000 -> 0x003FFFFF) */
    /* Index 0 对应虚拟地址 0x00000000 起始的 4MB */
    pd[0] = pt_phys | PAGE_PRESENT | PAGE_RW;

    /* [Higher-half Kernel] 映射高端 4MB (Virtual 0xC0000000 -> 0xC03FFFFF) */
    /* 0xC0000000 >> 22 = Index 768 */
    /* 让它指向同一个页表，这样访问 0xC0000000 就会落到物理 0x00000000 */
    pd[768] = pt_phys | PAGE_PRESENT | PAGE_RW;

    /* 5. 载入 CR3 并开启分页 */
    terminal_writestring("Loading CR3...\n");
    set_cr3(pd_phys);

    terminal_writestring("Enabling Paging...\n");
    enable_paging_bit();

    terminal_writestring("VMM initialized! Higher-half mapped at 0xC0000000.\n");
}
