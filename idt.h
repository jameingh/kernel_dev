#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// IDT门描述符结构
struct idt_entry {
    uint16_t base_low;      // 处理函数地址低16位
    uint16_t sel;           // 内核段选择子
    uint8_t always0;        // 总是0
    uint8_t flags;          // 类型标志
    uint16_t base_high;     // 处理函数地址高16位
} __attribute__((packed));

// IDT指针结构
struct idt_ptr {
    uint16_t limit;         // IDT大小
    uint32_t base;          // IDT基地址
} __attribute__((packed));

// 函数声明
void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

#endif
