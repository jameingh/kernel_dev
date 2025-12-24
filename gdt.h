#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// GDT段描述符结构
struct gdt_entry {
    uint16_t limit_low;     // 段限长低16位
    uint16_t base_low;      // 基地址低16位
    uint8_t base_middle;    // 基地址中间8位
    uint8_t access;         // 访问字节
    uint8_t granularity;     // 粒度和标志
    uint8_t base_high;      // 基地址高8位
} __attribute__((packed));

// GDT指针结构
struct gdt_ptr {
    uint16_t limit;         // GDT大小
    uint32_t base;          // GDT基地址
} __attribute__((packed));

// TSS 结构体定义 (用于 Ring 3 到 Ring 0 切换)
struct tss_entry_struct {
    uint32_t prev_tss;   // 如果使用硬件切换，这里是前一个 TSS 的链接
    uint32_t esp0;       // Ring 0 堆栈指针 (最重要！)
    uint32_t ss0;        // Ring 0 堆栈选择子
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

typedef struct tss_entry_struct tss_entry_t;

// 函数声明
void gdt_init(void);
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void tss_set_stack(uint32_t stack);

#endif
