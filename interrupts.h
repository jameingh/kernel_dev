#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

// 寄存器结构
struct registers {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

// 函数声明
void isr_init(void);
void irq_init(void);
void isr_handler(struct registers* regs);
void irq_handler(struct registers* regs);

#endif
