#include "terminal.h"
#include "gdt.h"
#include "idt.h"
#include "interrupts.h"

void kmain(void) {
    // 初始化终端：设置默认颜色、禁用硬件光标，准备文本输出
    terminal_initialize();
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("========================================\n");
    terminal_writestring("    Welcome to MyOS Kernel v2.0!\n");
    terminal_writestring("========================================\n\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    terminal_writestring("Initializing GDT...\n");
    gdt_init();
    terminal_writestring("GDT initialized successfully!\n\n");
    
    terminal_writestring("Initializing IDT...\n");
    // IDT：加载空表 → 注册 ISR/IRQ → 重映射 PIC → 初始化 PIT
    idt_init();
    isr_init();
    irq_init();
    pit_init(100);
    terminal_writestring("IDT initialized successfully!\n\n");
    
    terminal_writestring("System ready! Interrupts enabled.\n");
    terminal_writestring("Press any key to test keyboard interrupt...\n");
    
    // 启用中断：允许 CPU 响应外设与异常（此后可接收 IRQ0/IRQ1 等）
    asm volatile ("sti");
    
    
    while (1) {
        // 主循环
    }
}
