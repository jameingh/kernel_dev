#include "gdt.h"

struct gdt_entry gdt[6];
struct gdt_ptr gp;
tss_entry_t tss_entry;

extern void gdt_flush(uint32_t);  // 汇编函数声明

// 设置 GDT 表项
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    
    gdt[num].access = access;
}

// 初始化 TSS
static void write_tss(int num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t)&tss_entry;
    uint32_t limit = sizeof(tss_entry) - 1; // 修正：这是段限长，不是地址
    
    // 在 GDT 中记录 TSS (0x89: P=1, DPL=0, Type=9: Available 32-bit TSS)
    gdt_set_gate(num, base, limit, 0x89, 0x00);
    
    // 填充 TSS 结构体
    for(int i = 0; i < sizeof(tss_entry); i++) ((char*)&tss_entry)[i] = 0;
    
    tss_entry.ss0 = ss0;   // 内核数据段选择子
    tss_entry.esp0 = esp0; // 内核栈顶
    
    // 设置段选择子 (必须要设置，否则切换时崩溃)
    tss_entry.cs = 0x08 | 0x3;
    tss_entry.ss = 0x10 | 0x3;
    tss_entry.ds = 0x10 | 0x3;
    tss_entry.es = 0x10 | 0x3;
    tss_entry.fs = 0x10 | 0x3;
    tss_entry.gs = 0x10 | 0x3;
}

// 供调度器调用，更新当前进程的内核栈，以便中断发生时能正确切换回内核态
void tss_set_stack(uint32_t stack) {
    tss_entry.esp0 = stack;
}

// 初始化 GDT
void gdt_init(void) {
    gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gp.base = (uint32_t)&gdt;
    
    // 0: 空描述符
    gdt_set_gate(0, 0, 0, 0, 0);
    
    // 1: 内核代码段 (Base=0, Limit=4G, Ring0)
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    
    // 2: 内核数据段 (Base=0, Limit=4G, Ring0)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    
    // 3: 用户代码段 (Base=0, Limit=4G, Ring3)
    // Access: 0xFA = 1111 1010b (P=1, DPL=3, S=1, Type=Execute/Read)
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    
    // 4: 用户数据段 (Base=0, Limit=4G, Ring3)
    // Access: 0xF2 = 1111 0010b (P=1, DPL=3, S=1, Type=Read/Write)
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    
    // 5: TSS (Task State Segment)
    // 我们暂时把内核栈设为 0x90000 (bootloader初始栈)
    // 实际上多任务启动后，调度器会动态更新它
    write_tss(5, 0x10, 0x90000);
    
    // 加载 GDT 并在汇编中 ltr
    gdt_flush((uint32_t)&gp);
}
