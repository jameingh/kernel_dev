#include "idt.h"

// IDT 表（256 项）与指针（给 lidt 使用）
struct idt_entry idt[256];
struct idt_ptr idtp;

extern void idt_flush(uint32_t);  // 汇编函数声明

// 设置 IDT 表项：
// - base: 处理例程地址（低/高 16 位分拆）
// - sel : 段选择子（通常为内核代码段 0x08）
// - flags: 0x8E 表示 32 位中断门、DPL=0、P=1
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

// 初始化 IDT：
// - 设置 idtp.limit/base
// - 清空 256 项（占位为 0）
// - 通过 lidt 加载，让 CPU 使用新的 IDT
void idt_init(void) {
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint32_t)&idt;
    
    // 清空IDT
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    // 加载IDT
    idt_flush((uint32_t)&idtp);
}
