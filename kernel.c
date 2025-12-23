#include "terminal.h"
#include "gdt.h"
#include "idt.h"
#include "interrupts.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "process.h"

/* Forward declarations */
void task_a(void);
void task_b(void);

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
    
    terminal_writestring("Initializing PMM...\n");
    pmm_init();

    terminal_writestring("Initializing VMM...\n");
    vmm_init();

    terminal_writestring("Initializing Heap...\n");
    kheap_init();

    terminal_writestring("Testing Heap...\n");
    void* ptrA = kmalloc(10);
    void* ptrB = kmalloc(20);
    terminal_writestring("Malloc A: "); if(ptrA) terminal_writestring("OK ");
    terminal_writestring("Malloc B: "); if(ptrB) terminal_writestring("OK\n");
    kfree(ptrA);
    kfree(ptrB);
    terminal_writestring("Free A&B OK\n\n");

    /* 多任务测试 */
    process_init(); /* 初始化，当前变为 Idle 任务 */

    process_create(task_a, "Task A");
    process_create(task_b, "Task B");
    
    terminal_writestring("Tasks created. Entering infinite loop...\n");

    terminal_writestring("IDT initialized successfully!\n\n");
    status_refresh();
    
    terminal_writestring("System ready! Interrupts enabled.\n");
    terminal_writestring("Press any key to test keyboard interrupt...\n");
    
    // 启用中断 = 开始调度
    asm volatile("sti");

    // 主线程 (Idle)
    while(1) {
         asm volatile("hlt");
    }
}

void task_a(void) {
    while(1) {
        terminal_putchar('A');
        /* 简单的延时循环，模拟耗时任务 */
        for(volatile int i=0; i<10000000; i++); 
    }
}

void task_b(void) {
    while(1) {
        terminal_putchar('B');
        for(volatile int i=0; i<10000000; i++);
    }
}
