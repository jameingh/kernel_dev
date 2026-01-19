#include "terminal.h"
#include "gdt.h"
#include "idt.h"
#include "interrupts.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "process.h"
#include "initrd.h"
#include "shell.h"

/* Forward declarations */
void task_a(void);
void task_b(void);
void user_task(void);

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

    /* 文件系统测试 */
    terminal_writestring("Initializing InitRD...\n");
    fs_root = initrd_init();
    
    if (fs_root) {
        terminal_writestring("Listing files in /:\n");
        fs_node_t* node = vfs_finddir(fs_root, "hello.txt");
        if (node) {
            terminal_writestring("Found: hello.txt\n");
            uint8_t buf[32];
            uint32_t sz = vfs_read(node, 0, 32, buf);
            buf[sz] = 0; // Null terminate
            terminal_writestring("Content: ");
            terminal_writestring((char*)buf);
            terminal_putchar('\n');
        } else {
            terminal_writestring("File hello.txt not found!\n");
        }
    }

    /* 多任务测试 */
    process_init(); /* 初始化，当前变为 Idle 任务 */

    process_create(task_a, "Task A");
    process_create(task_b, "Task B");
    process_create_user(user_task, "User Task");
    
    terminal_writestring("Tasks created. Entering infinite loop...\n");

    terminal_writestring("IDT initialized successfully!\n\n");
    status_refresh();
    
    /* 初始化 Shell */
    shell_init();

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
    terminal_writestring("Task A started.\n");
    while(1) {
        /* 内核态允许使用 hlt 以节省 CPU */
        asm volatile("hlt");
    }
}

void task_b(void) {
    terminal_writestring("Task B started.\n");
    while(1) {
        asm volatile("hlt");
    }
}

void user_task(void) {
    char* msg = " [Syscall from Ring 3!] ";
    // 使用汇编发起系统调用
    asm volatile (
        "mov $1, %%eax\n"
        "mov %0, %%ebx\n"
        "int $0x80\n"
        : : "r"(msg) : "eax", "ebx"
    );

    while(1) {
        // 使用汇编发起 Sleep 系统调用
        // EAX = 3 (sleep), EBX = 500 (ms)
        asm volatile (
            "mov $3, %%eax\n"
            "mov $500, %%ebx\n"
            "int $0x80\n"
            : : : "eax", "ebx"
        );

        // 睡醒后打印一下，证明运行了
        // 使用系统调用号 1，在终端输出一个字符串 "."
        // 约定：EAX = 系统调用号，EBX = 第一个参数（这里是字符串指针）
        // 系统调用号1和3的定义在 syscall.c 的 syscall_handler() 方法中
        asm volatile (
            "mov $1, %%eax\n"      // 将立即数 1 写入寄存器 EAX，表示要调用的系统调用号为 1
            "mov %0, %%ebx\n"      // 将下面输入约束 %0 对应的地址写入 EBX，作为系统调用的第一个参数
            "int $0x80\n"          // 触发软中断 0x80，CPU 从用户态陷入内核态，进入系统调用处理例程
            :                      // 输出操作数列表为空：这段内联汇编不直接向 C 代码返回值
            : "r"(".")             // 输入操作数：将字符串常量 "." 的地址放入某个通用寄存器，作为占位符 %0
            : "eax", "ebx"         // 告诉编译器本段汇编会修改 EAX、EBX 寄存器，避免错误优化
        );
    }
}
