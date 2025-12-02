#include "interrupts.h"
#include "terminal.h"
#include "idt.h"

// 端口输出函数
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

// 中断服务例程
void isr_handler(struct registers* regs) {
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;

    // 处理时钟中断 (IRQ0 = 32)
    if (regs->int_no == 32) {
        // 暂时屏蔽视觉更新，排除干扰
        return;
    }

    // 打印 "INT: "
    vga[0] = (uint16_t)'I' | 0x0F00;
    vga[1] = (uint16_t)'N' | 0x0F00;
    vga[2] = (uint16_t)'T' | 0x0F00;
    vga[3] = (uint16_t)':' | 0x0F00;
    vga[4] = (uint16_t)' ' | 0x0F00;

    // 打印中断号 (十六进制)
    const char hex[] = "0123456789ABCDEF";
    vga[5] = (uint16_t)hex[(regs->int_no >> 4) & 0xF] | 0x0F00;
    vga[6] = (uint16_t)hex[regs->int_no & 0xF] | 0x0F00;

    // 如果是断点中断 (INT 3)
    if (regs->int_no == 3) {
        const char* msg = " Breakpoint (HALT)";
        for(int i=0; msg[i]; i++) {
            vga[7+i] = (uint16_t)msg[i] | 0x0A00; // 亮绿色
        }
        
        // 调试关键点：处理完INT 3后直接死循环，不返回！
        // 如果屏幕稳住了，说明是 iret 返回时出的问题。
        while(1) {
            asm volatile ("hlt");
        }
    }

    // 其他异常，打印 " EXCEPTION HALT" 并死循环
    const char* msg = " EXCEPTION HALT";
    for(int i=0; msg[i]; i++) {
        vga[7+i] = (uint16_t)msg[i] | 0x0C00; // 亮红色
    }
    
    while(1) {
        asm volatile ("hlt");
    }
}


// IRQ处理例程
void irq_handler(struct registers* regs) {
    // 发送EOI信号给PIC
    if (regs->int_no >= 40) {
        outb(0xA0, 0x20);  // 从PIC
    }
    outb(0x20, 0x20);      // 主PIC
    
    // 忽略时钟中断(IRQ0)的打印，防止刷屏
    if (regs->int_no == 32) return;

    terminal_writestring("Received IRQ: ");
    
    char hex_chars[] = "0123456789ABCDEF";
    char int_str[3];
    int_str[0] = hex_chars[(regs->int_no >> 4) & 0xF];
    int_str[1] = hex_chars[regs->int_no & 0xF];
    int_str[2] = '\0';
    
    terminal_writestring(int_str);
    terminal_putchar('\n');
}


// 初始化ISR

void isr_init(void) {
    // 设置所有ISR
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);
    idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);
    idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);
    idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);
    idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);
    idt_set_gate(9, (uint32_t)isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);
}

// 端口输入函数
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)

#define ICW1_ICW4	0x01		/* ICW4 (not) needed */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */

#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */

// 重新映射PIC
void pic_remap(int offset1, int offset2) {
    unsigned char a1, a2;

    a1 = inb(PIC1_DATA);                        // 保存屏蔽字
    a2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // 启动初始化序列 (ICW1)
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
    io_wait();
    outb(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
    io_wait();

    outb(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    io_wait();
    outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    outb(PIC1_DATA, ICW4_8086);               // ICW4: have the PICs use 8086 mode (and not 8080 mode)
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, a1);   // 恢复屏蔽字
    outb(PIC2_DATA, a2);
}

// 初始化IRQ
void irq_init(void) {
    // 重新映射PIC，将IRQ0-15映射到IDT 32-47
    pic_remap(0x20, 0x28);

    // 设置IRQ
    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2, 0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3, 0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4, 0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5, 0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6, 0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7, 0x08, 0x8E);
    idt_set_gate(40, (uint32_t)irq8, 0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9, 0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);
}
