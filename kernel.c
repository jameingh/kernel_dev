#include "terminal.h"
#include "gdt.h"
#include "idt.h"
#include "interrupts.h"

void kmain(void) {
    // 初始化终端
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
    idt_init();
    isr_init();
    irq_init();
    terminal_writestring("IDT initialized successfully!\n\n");
    
    terminal_writestring("System ready! Interrupts enabled.\n");
    terminal_writestring("Press any key to test keyboard interrupt...\n");
    
    // 启用中断
    asm volatile ("sti");
    
    // 触发一个中断来测试
    terminal_writestring("\nTriggering interrupt 3 (breakpoint)...\n");
    asm volatile ("int $3");
    
    while (1) {
        // 主循环
    }
}
