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
        // 设置为 User 权限，具体是否 Present 由 PT 决定
        pd[i] = 0x00000006; /* RW, User, Not Present */
    }

    /* 3. 填充第一个页表 (Identity Mapping 0-4MB) */
    /* 每一个 PTE 映射 4KB */
    /* 0x00000000 -> 0x00000000, 0x00001000 -> 0x00001000 ... */
    for (int i = 0; i < 1024; i++) {
        // 恒等映射区：允许用户态访问（包含了 kernel 代码和 user_task）
        pt[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    }

    /* 4. 填写页目录项 (Reference the Page Table) */

    /* [Identity Mapping] 映射低端 4MB (Virtual 0x00000000 -> 0x003FFFFF) */
    pd[0] = pt_phys | PAGE_PRESENT | PAGE_RW | PAGE_USER;

    /* [Higher-half Kernel] 映射高端 4MB (Virtual 0xC0000000 -> 0xC03FFFFF) */
    // 内核高半空间可以保持为 Supervisor (不加 PAGE_USER)
    pd[768] = pt_phys | PAGE_PRESENT | PAGE_RW;

    /* [Heap] 映射 1MB 给堆 (Virtual 0xD0000000) */
    /* Index = 0xD0000000 >> 22 = 832 */
    uint32_t heap_pt_phys = pmm_alloc_page();
    if (heap_pt_phys != 0) {
        uint32_t* heap_pt = (uint32_t*)heap_pt_phys;
        // PDE 必须开启 User 位
        pd[832] = heap_pt_phys | PAGE_PRESENT | PAGE_RW | PAGE_USER;
        
        /* 映射 256 个页 (1MB) */
        for (int i = 0; i < 256; i++) {
            uint32_t phys = pmm_alloc_page();
            // 堆内存允许用户态访问（因为用户栈在这里）
            heap_pt[i] = phys | PAGE_PRESENT | PAGE_RW | PAGE_USER;
        }
    }

    /* 5. 载入 CR3 并开启分页 */
    terminal_writestring("Loading CR3...\n");
    set_cr3(pd_phys);

    terminal_writestring("Enabling Paging...\n");
    enable_paging_bit();

    terminal_writestring("VMM initialized! Higher-half mapped at 0xC0000000.\n");
}
