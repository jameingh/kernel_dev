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

/**
 * @brief 内核入口函数 (Kernel Entry Point)
 * 
 * 这是内核从汇编引导代码 (boot.s) 跳转过来后执行的第一个 C 函数。
 * 它负责按特定顺序初始化硬件抽象层和各个子系统。
 * 
 * @see [kernel_entry.md](doc/kernel_entry.md) - 初始化流程图解
 */
void kmain(void) {
    /* 1. 终端初始化
     * 最先初始化，以便后续步骤可以打印日志信息。
     * 清屏、设置默认颜色、禁用硬件光标。
     */
    terminal_initialize();
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("========================================\n");
    terminal_writestring("    Welcome to MyOS Kernel v2.0!\n");
    terminal_writestring("========================================\n\n");
    
    /* 2. GDT (Global Descriptor Table) 初始化
     * 必须在 IDT 和内存管理之前完成。
     * 它定义了内核态和用户态的代码段/数据段，是保护模式的基础。
     */
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    terminal_writestring("Initializing GDT...\n");
    gdt_init();
    terminal_writestring("GDT initialized successfully!\n\n");
    
    /* 3. 中断系统初始化
     * - idt_init: 加载空的 IDT 表。
     * - isr_init: 注册 CPU 异常处理函数 (如 Page Fault)。
     * - irq_init: 重映射 PIC (可编程中断控制器) 并注册硬件中断 (如键盘、时钟)。
     * - pit_init: 初始化定时器，用于后续的任务调度 (Time Slicing)。
     */
    terminal_writestring("Initializing IDT...\n");
    idt_init();
    isr_init();
    irq_init();
    pit_init(100); /* 100Hz = 每 10ms 触发一次时钟中断 */
    
    /* 4. 内存管理初始化
     * - PMM (Physical Memory Manager): 管理物理页框的分配/释放。
     * - VMM (Virtual Memory Manager): 建立页表，开启分页机制。
     * - Heap: 在 VMM 之上建立内核堆，支持 kmalloc/kfree。
     */
    terminal_writestring("Initializing PMM...\n");
    pmm_init();

    terminal_writestring("Initializing VMM...\n");
    vmm_init();

    terminal_writestring("Initializing Heap...\n");
    kheap_init();

    /* 堆分配测试 */
    terminal_writestring("Testing Heap...\n");
    void* ptrA = kmalloc(10);
    void* ptrB = kmalloc(20);
    terminal_writestring("Malloc A: "); if(ptrA) terminal_writestring("OK ");
    terminal_writestring("Malloc B: "); if(ptrB) terminal_writestring("OK\n");
    kfree(ptrA);
    kfree(ptrB);
    terminal_writestring("Free A&B OK\n\n");

    /* 5. 文件系统初始化
     * 初始化 InitRD (Initial Ramdisk)，并挂载 VFS (虚拟文件系统)。
     * 这使得内核可以读取打包在镜像中的文件 (如 hello.txt)。
     */
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

    /* 6. 多任务子系统初始化
     * - process_init: 将当前执行流 (kmain) 包装为 PID 0 的 Idle 进程。
     * - process_create: 创建新的内核线程。
     * - process_create_user: 创建用户态进程 (Ring 3)。
     */
    terminal_writestring("Tasks created. Entering infinite loop...\n");
    process_init(); 

    process_create(task_a, "Task A");
    process_create(task_b, "Task B");
    process_create_user(user_task, "User Task");
    
    terminal_writestring("IDT initialized successfully!\n\n");
    
    /* 刷新底部状态栏 (如果有的话) */
    status_refresh();
    
    /* 7. 初始化 Shell
     * 这是一个简单的交互式命令行环境。
     */
    shell_init();

    terminal_writestring("System ready! Interrupts enabled.\n");
    terminal_writestring("Press any key to test keyboard interrupt...\n");
    
    /* 8. 开启中断，启动调度
     * 这里的 sti (Set Interrupt Flag) 指令一旦执行，CPU 就开始响应中断。
     * 当第一次时钟中断到来时，scheduler 就会介入，开始任务切换。
     */
    asm volatile("sti");

    /* 9. Idle Loop (主循环)
     * 当没有其他任务可运行时，调度器会切换回这里。
     * hlt 指令让 CPU 暂停直到下一个中断，节省能源。
     */
    while(1) {
         asm volatile("hlt");
    }
}

/**
 * @brief 内核任务 A
 * 简单的无限循环任务，用于演示多任务切换。
 */
void task_a(void) {
    terminal_writestring("Task A started.\n");
    while(1) {
        /* 内核态允许使用 hlt 以节省 CPU，
           因为只有中断能将其唤醒，而中断会自动发生。 */
        asm volatile("hlt");
    }
}

/**
 * @brief 内核任务 B
 */
void task_b(void) {
    terminal_writestring("Task B started.\n");
    while(1) {
        asm volatile("hlt");
    }
}

/**
 * @brief 用户态任务 (Ring 3)
 * 
 * 这个任务运行在用户模式 (Ring 3)。
 * 它不能直接执行特权指令 (如 hlt, cli, sti) 或直接访问硬件端口。
 * 它必须通过系统调用 (System Call) 请求内核服务。
 */
void user_task(void) {
    char* msg = " [Syscall from Ring 3!] ";
    
    /* 发起系统调用测试：打印字符串
     * EAX = 1 (系统调用号: sys_print)
     * EBX = 字符串地址
     * int 0x80 = 触发软中断，陷入内核
     */
    asm volatile (
        "mov $1, %%eax\n"
        "mov %0, %%ebx\n"
        "int $0x80\n"
        : : "r"(msg) : "eax", "ebx"
    );

    while(1) {
        /* 发起系统调用：睡眠 (Sleep)
         * EAX = 3 (系统调用号: sys_sleep)
         * EBX = 500 (参数: 毫秒)
         */
        asm volatile (
            "mov $3, %%eax\n"
            "mov $500, %%ebx\n"
            "int $0x80\n"
            : : : "eax", "ebx"
        );

        /* 睡醒后打印一个点，表明任务处于活跃状态 */
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
