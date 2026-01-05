#include "interrupts.h"
#include "terminal.h"
#include "idt.h"
#include "process.h"
#include "syscall.h"
#include "shell.h"
// 本文件负责：
// - 异常处理入口（isr_handler）：任何异常均在屏幕顶行输出异常号并停机，便于早期诊断
// - IRQ 分发（irq_handler）：按向量号处理 PIT(IRQ0)、键盘(IRQ1) 等，并向 PIC 发送 EOI
// - PIT 初始化与心跳状态栏：在第一行右侧定宽绘制 Hz/Ticks/Keys/Caps/Shift
// - 键盘扫描码解析（Set1）：支持 Enter/Backspace/Shift/Caps，字母大小写依据 Shift XOR Caps

// 端口输出函数
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

// 中断服务例程（异常路径）：
// 说明：异常多发生在终端初始化之前，为保证可视化，使用直写 VGA 顶行而非终端 API。
// 行为：显示 "EXC XX"（两位十六进制异常号），随后进入 hlt 死循环，防止屏幕抖动。
void isr_handler(struct registers* regs) {
    if (regs->int_no == 128) {
        syscall_handler(regs);
        return;
    }

    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
    const char hex[] = "0123456789ABCDEF";
    vga[0] = (uint16_t)'E' | 0x0C00;
    vga[1] = (uint16_t)'X' | 0x0C00;
    vga[2] = (uint16_t)'C' | 0x0C00;
    vga[3] = (uint16_t)' ' | 0x0C00;
    vga[4] = (uint16_t)hex[(regs->int_no >> 4) & 0xF] | 0x0C00;
    vga[5] = (uint16_t)hex[regs->int_no & 0xF] | 0x0C00;
    while(1) { asm volatile ("hlt"); }
}


// 运行时状态：
// - pit_ticks：PIT 触发次数计数（随系统运行递增）
// - pit_rate ：当前 PIT 频率（Hz），用于状态栏展示
// - key_count：收到按键的总次数（仅统计 make 码）
// - shift_on_global / caps_on_global：用于状态栏展示的修饰键状态
static volatile uint32_t pit_ticks = 0;
static volatile uint32_t pit_rate = 100;
static volatile uint32_t key_count = 0;
static volatile uint8_t shift_on_global = 0;
static volatile uint8_t caps_on_global = 0;
// 在第一行固定区域绘制状态栏（定宽、定域，避免滚屏与大面积刷新）：
// 格式："Hz:xxx Keys:xxxx MemFree:xxxxx"
extern uint32_t pmm_free_pages(void);
static inline void draw_status(void) {
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
    char buf[32];
    int p = 0;
    buf[p++] = 'H'; buf[p++] = 'z'; buf[p++] = ':';
    uint32_t v = pit_rate;
    char num3[3]; for (int i = 2; i >= 0; i--) { num3[i] = '0' + (v % 10); v /= 10; }
    for (int i = 0; i < 3; i++) buf[p++] = num3[i];
    buf[p++] = ' ';
    buf[p++] = 'K'; buf[p++] = 'e'; buf[p++] = 'y'; buf[p++] = 's'; buf[p++] = ':';
    v = key_count; char num4[4]; for (int i = 3; i >= 0; i--) { num4[i] = '0' + (v % 10); v /= 10; }
    for (int i = 0; i < 4; i++) buf[p++] = num4[i];
    buf[p++] = ' ';
    buf[p++] = 'M'; buf[p++] = 'e'; buf[p++] = 'm'; buf[p++] = 'F'; buf[p++] = 'r'; buf[p++] = 'e'; buf[p++] = 'e'; buf[p++] = ':';
    uint32_t mf = pmm_free_pages();
    char num5[5]; for (int i = 4; i >= 0; i--) { num5[i] = '0' + (mf % 10); mf /= 10; }
    for (int i = 0; i < 5; i++) buf[p++] = num5[i];
    uint16_t attr = 0x0200;
    for (int i = 50; i < 80; i++) {
        vga[0 * 80 + i] = (uint16_t)' ' | attr;
    }
    int start = 80 - p;
    if (start < 50) start = 50;
    for (int i = 0; i < p; i++) {
        vga[0 * 80 + start + i] = (uint16_t)buf[i] | attr;
    }
}

void status_refresh(void) {
    draw_status();
}

// 将键盘扫描码（Set1）转换为 ASCII 字符：
// - 回车 0x1C → '\n'；退格 0x0E → '\b'
// - 字母大小写由 (shift XOR caps) 决定；数字与符号暂不处理 Shift 变体
static char translate_scancode(uint8_t sc, uint8_t shift, uint8_t caps) {
    switch (sc) {
        case 0x0E: return '\b';
        case 0x02: return '1';
        case 0x03: return '2';
        case 0x04: return '3';
        case 0x05: return '4';
        case 0x06: return '5';
        case 0x07: return '6';
        case 0x08: return '7';
        case 0x09: return '8';
        case 0x0A: return '9';
        case 0x0B: return '0';
        case 0x0C: return '-';
        case 0x0D: return '=';
        case 0x10: return (shift ^ caps) ? 'Q' : 'q';
        case 0x11: return (shift ^ caps) ? 'W' : 'w';
        case 0x12: return (shift ^ caps) ? 'E' : 'e';
        case 0x13: return (shift ^ caps) ? 'R' : 'r';
        case 0x14: return (shift ^ caps) ? 'T' : 't';
        case 0x15: return (shift ^ caps) ? 'Y' : 'y';
        case 0x16: return (shift ^ caps) ? 'U' : 'u';
        case 0x17: return (shift ^ caps) ? 'I' : 'i';
        case 0x18: return (shift ^ caps) ? 'O' : 'o';
        case 0x19: return (shift ^ caps) ? 'P' : 'p';
        case 0x1A: return '[';
        case 0x1B: return ']';
        case 0x1C: return '\n';
        case 0x1E: return (shift ^ caps) ? 'A' : 'a';
        case 0x1F: return (shift ^ caps) ? 'S' : 's';
        case 0x20: return (shift ^ caps) ? 'D' : 'd';
        case 0x21: return (shift ^ caps) ? 'F' : 'f';
        case 0x22: return (shift ^ caps) ? 'G' : 'g';
        case 0x23: return (shift ^ caps) ? 'H' : 'h';
        case 0x24: return (shift ^ caps) ? 'J' : 'j';
        case 0x25: return (shift ^ caps) ? 'K' : 'k';
        case 0x26: return (shift ^ caps) ? 'L' : 'l';
        case 0x27: return ';';
        case 0x28: return '\'';
        case 0x29: return '`';
        case 0x2C: return (shift ^ caps) ? 'Z' : 'z';
        case 0x2D: return (shift ^ caps) ? 'X' : 'x';
        case 0x2E: return (shift ^ caps) ? 'C' : 'c';
        case 0x2F: return (shift ^ caps) ? 'V' : 'v';
        case 0x30: return (shift ^ caps) ? 'B' : 'b';
        case 0x31: return (shift ^ caps) ? 'N' : 'n';
        case 0x32: return (shift ^ caps) ? 'M' : 'm';
        case 0x33: return ',';
        case 0x34: return '.';
        case 0x35: return '/';
        case 0x39: return ' ';
        default: return 0;
    }
}

// 初始化 PIT，设置通道0为方波模式（0x36），频率 hz
// divisor = 1193180 / hz：PIT 时钟为 1.19318 MHz
void pit_init(uint32_t hz) {
    pit_rate = hz;
    uint32_t divisor = 1193180 / hz;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

// IRQ 分发：
// - 先发 EOI（若为从 PIC 中断，需先向 0xA0 再向 0x20）
// - 处理定时器与键盘分支；其余 IRQ 打印向量号
struct registers* irq_handler(struct registers* regs) {
    if (regs->int_no >= 40) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);

    // IRQ0：定时器心跳，周期刷新状态栏（降低刷新频率避免抖动）
    if (regs->int_no == 32) {
        pit_ticks++;
        if ((pit_ticks % 10) == 0) {
            draw_status();
        }
        /* [关键改动] 调用调度器 */
        return schedule(regs);
    }

    static uint8_t shift_on = 0;
    static uint8_t caps_on = 0;
    // IRQ1：键盘。处理 Shift/Caps 修饰键状态，过滤 break 码，回显 make 码字符。
    if (regs->int_no == 33) {
        uint8_t sc = inb(0x60);
        if (sc == 0x2A || sc == 0x36) { shift_on = 1; return regs; }
        if (sc == 0xAA || sc == 0xB6) { shift_on = 0; return regs; }
        if (sc == 0x3A) { caps_on ^= 1; caps_on_global = caps_on; return regs; }
        if (sc & 0x80) return regs;
        char c = translate_scancode(sc, shift_on, caps_on);
        shift_on_global = shift_on;
        if (c) { shell_input(c); key_count++; }
        return regs;
    }

    terminal_writestring("Received IRQ: ");
    char hex_chars[] = "0123456789ABCDEF";
    char int_str[3];
    int_str[0] = hex_chars[(regs->int_no >> 4) & 0xF];
    int_str[1] = hex_chars[regs->int_no & 0xF];
    int_str[2] = '\0';
    terminal_writestring(int_str);
    terminal_putchar('\n');
    return regs;
}


// 初始化 ISR：为 0–31 号异常设置 IDT 门（0x8E 中断门，选择子 0x08）
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
    idt_set_gate(128, (uint32_t)isr128, 0x08, 0xEE); // DPL=3 for syscalls!
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
    unsigned char a1 = inb(PIC1_DATA);
    unsigned char a2 = inb(PIC2_DATA);
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC1_DATA, offset1);
    io_wait();
    outb(PIC2_DATA, offset2);
    io_wait();
    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();
    outb(PIC1_DATA, 0xF8);
    outb(PIC2_DATA, 0xFF);
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
