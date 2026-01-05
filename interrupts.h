/**
 * interrupts.h - 内核中断与异常处理框架
 * 
 * 该文件定义了中断发生时的寄存器现场结构，并声明了汇编层与 C 层之间的处理接口。
 */
#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

/**
 * struct registers - 中断现场寄存器结构
 * 
 * 当中断发生时，汇编桩 (isr.asm) 会将 CPU 的寄存器状态压入栈中。
 * 这个结构体的布局必须与汇编中压栈的顺序完全一致。
 */
struct registers {
    // 由汇编桩手动压入的段寄存器 (push ds, es, fs, gs)
    uint32_t gs, fs, es, ds;
    
    // 由 pusha 指令压入的通用寄存器
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    
    // 中断号和错误码 (由汇编模板压入)
    uint32_t int_no, err_code;
    
    // 以下由处理器在发生中断时自动压入 (如果是从用户态进入，还包含 useresp 和 ss)
    uint32_t eip, cs, eflags, useresp, ss;
} __attribute__((packed));

/**
 * ISR (Interrupt Service Routine) - 中断服务例程
 * 通常指由 CPU 内部触发的异常（如除零、页错误）或软中断。
 * 向量号 0-31 为系统预留异常，128 (0x80) 用于系统调用。
 */
// ISR函数声明 (在isr.asm中定义)
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);
extern void isr128(void);


/**
 * IRQ (Interrupt Request) - 中断请求
 * 由外部硬件设备（如时钟、键盘、磁盘）触发的异步信号。
 * 经过 PIC (可编程中断控制器) 重映射后，对应 IDT 中的 32-47 号向量。
 */
// IRQ函数声明 (在isr.asm中定义)
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

/**
 * isr_init - 初始化中断服务例程 (ISR)
 * 向 IDT 中注册 0-31 号系统异常处理器，以及 128 号系统调用处理器。
 */
void isr_init(void);

/**
 * irq_init - 初始化中断请求 (IRQ)
 * 重新映射 PIC，并向 IDT 注册 32-47 号硬件中断处理器。
 */
void irq_init(void);

/**
 * pit_init - 初始化可编程间隔定时器 (PIT)
 * @hz: 期望的中断频率 (如 100 表示每秒 100 次中断)。
 */
void pit_init(uint32_t hz);

/**
 * isr_handler - C 语言异常处理入口
 * @regs: 指向栈中保存的寄存器现场的指针。
 * 返回值: 修改后或切换后的寄存器上下文指针。
 */
struct registers* isr_handler(struct registers* regs);

/**
 * irq_handler - C 语言硬件中断处理入口
 * @regs: 指向栈中保存的寄存器现场的指针。
 * 返回值: 修改后或切换后的寄存器上下文指针 (用于进程调度)。
 */
struct registers* irq_handler(struct registers* regs);

/**
 * status_refresh - 刷新屏幕状态栏
 * 在屏幕顶部绘制当前系统运行状态 (如 Hz, 内存, 按键数等)。
 */
void status_refresh(void);

#endif
