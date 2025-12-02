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

// 函数声明
void gdt_init(void);
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

#endif
