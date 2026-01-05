#include "process.h"
#include "heap.h"
#include "terminal.h"
#include <stddef.h>
#include "gdt.h"

/* 全局进程链表 */
static process_t* process_list = NULL;
static process_t* current_process = NULL;
static uint32_t next_pid = 1;

void process_init(void) {
    /* 创建代表当前执行流 (Kernel Main) 的 PCB */
    /* 注意：我们不需要给它分配栈，因为我们已经在它的栈里运行了 */
    /* 当发生第一次切换时，它的 ESP 会被保存 */
    
    process_t* main_proc = (process_t*)kmalloc(sizeof(process_t));
    main_proc->pid = 0;
    main_proc->esp = 0; /* 暂时未知，切出时会保存 */
    
    /* 简单的字符串复制 (避免引入 string.h) */
    const char* name = "Kernel_Idle";
    for(int i=0; i<PROCESS_NAME_LEN && name[i]; i++) main_proc->name[i] = name[i];
    
    main_proc->next = main_proc; /* 循环链表：自己指向自己 */
    main_proc->kernel_stack_top = 0x90000; // 初始栈
    main_proc->state = STATE_READY;
    main_proc->sleep_ticks = 0;
    
    process_list = main_proc;
    current_process = main_proc;
    
    terminal_writestring("Multitasking initialized. Kernel is PID 0.\n");
}

process_t* process_create(void (*entry_point)(void), const char* name) {
    /* 1. 分配 PCB */
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));
    proc->pid = next_pid++;
    
    int i = 0;
    for(; i < PROCESS_NAME_LEN-1 && name[i]; i++) proc->name[i] = name[i];
    proc->name[i] = 0;
    
    /* 2. 分配内核栈 (4KB) */
    /* 注意：栈是从高地址向下增长的，所以 ESP 初始值要是 栈底+大小 */
    void* stack = kmalloc(4096);
    uint32_t esp = (uint32_t)stack + 4096;
    
    /* 3. 在栈上伪造中断现场 (Interrupt Frame) */
    /* 使得当 CPU 切换到这个栈并执行 `popa` + `iret` 后，能够“返回”到 entry_point */
    
    /* 栈布局必须严格匹配 isr.asm 中的 push 顺序：
       [EIP] [CS] [EFLAGS] [ESP(User)] [SS(User)] (如果是 Ring3) -> 这里是 Ring0 只有前面三个
       但是我们是用 IRET 返回的，IRET 会弹出 EIP, CS, EFLAGS.
       
       等等，我们的 `isr_common_stub` 是这样的：
       pusha
       push ds, es, fs, gs
       push esp (param)
       call handler
       
       所以切换栈指针时，新栈必须包含：
       [EFLAGS] [CS] [EIP] (IRET 需要)
       [ERR_CODE] [INT_NO] (为了平衡 add esp, 8)
       [EAX, ECX, EDX, EBX, ESP(ignored), EBP, ESI, EDI] (POPA 需要)
       [DS, ES, FS, GS] (POP 需要)
    */
    
    uint32_t* stack_ptr = (uint32_t*)esp;
    
    /* IRET Frame */
    *(--stack_ptr) = 0x202;         /* EFLAGS (Interrupts Enabled) */
    *(--stack_ptr) = 0x08;          /* CS (Kernel Code) */
    *(--stack_ptr) = (uint32_t)entry_point; /* EIP */
    
    /* Error Code & Int No (Dummy values to match `add esp, 8`) */
    *(--stack_ptr) = 0; /* Int No */
    *(--stack_ptr) = 0; /* Err Code */
    
    /* POPA Frame (Initial register values, all 0) */
    *(--stack_ptr) = 0; /* EAX */
    *(--stack_ptr) = 0; /* ECX */
    *(--stack_ptr) = 0; /* EDX */
    *(--stack_ptr) = 0; /* EBX */
    *(--stack_ptr) = 0; /* ESP (Ignored by POPA) */
    *(--stack_ptr) = 0; /* EBP */
    *(--stack_ptr) = 0; /* ESI */
    *(--stack_ptr) = 0; /* EDI */
    
    /* Segment Registers */
    *(--stack_ptr) = 0x10; /* DS */
    *(--stack_ptr) = 0x10; /* ES */
    *(--stack_ptr) = 0x10; /* FS */
    *(--stack_ptr) = 0x10; /* GS */
    
    /* 保存最终的 ESP 到 PCB */
    proc->esp = (uint32_t)stack_ptr;
    proc->kernel_stack_top = esp;
    proc->state = STATE_READY;
    proc->sleep_ticks = 0;
    
    /* 4. 插入链表 (插入到 head 后面) */
    proc->next = process_list->next;
    process_list->next = proc;
    
    return proc;
}

process_t* process_create_user(void (*entry_point)(void), const char* name) {
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));
    proc->pid = next_pid++;
    
    int i = 0;
    for(; i < PROCESS_NAME_LEN-1 && name[i]; i++) proc->name[i] = name[i];
    proc->name[i] = 0;
    
    /* 1. 分配独立的内核栈 (用于中断发生时切换) */
    void* kstack = kmalloc(4096);
    uint32_t kstack_top = (uint32_t)kstack + 4096;
    proc->kernel_stack_top = kstack_top;

    /* 2. 分配独立的用户栈 (用户程序平时使用的栈) */
    void* ustack = kmalloc(4096);
    uint32_t ustack_top = (uint32_t)ustack + 4096;

    uint32_t* stack_ptr = (uint32_t*)kstack_top;
    
    /* Ring 3 IRET Frame (SS, ESP, EFLAGS, CS, EIP) */
    *(--stack_ptr) = 0x23;          /* SS (User Data 0x20 | 3) */
    *(--stack_ptr) = ustack_top;    /* User ESP (现在指向独立的用户栈) */
    *(--stack_ptr) = 0x202;         /* EFLAGS (IF=1) */
    *(--stack_ptr) = 0x1B;          /* CS (User Code 0x18 | 3) */
    *(--stack_ptr) = (uint32_t)entry_point; /* EIP */
    
    /* Dummy Error Code & Int No */
    *(--stack_ptr) = 0; 
    *(--stack_ptr) = 0; 
    
    /* POPA Frame (通用寄存器初始清零) */
    for(int j=0; j<8; j++) *(--stack_ptr) = 0;
    
    /* 段寄存器 (Ring 3) */
    for(int j=0; j<4; j++) *(--stack_ptr) = 0x23; 
    
    proc->esp = (uint32_t)stack_ptr;
    proc->state = STATE_READY;
    proc->sleep_ticks = 0;
    
    /* 插入链表 */
    proc->next = process_list->next;
    process_list->next = proc;
    
    return proc;
}

struct registers* schedule(struct registers* current_regs) {
    if (!current_process) return current_regs;
    
    /* 1. 保存当前任务的栈指针 */
    /* 注意：current_regs 其实就是指向旧栈栈顶的指针（在 isr.asm 里 push esp 传递过来的）*/
    /* 但是 isr.asm 里是在 push esp, call handler 之前 push 的。
       iret 前的栈顶实际上就是 current_regs 指向的位置（如果忽略参数 push）。
       准确地说，current_regs 指向的是 `push esp` 压入的那个值，也就是 `push ds` 之后的 ESP。
       而我们的 `mov esp, eax` 会把 ESP 设为 EAX。
       如果我不切换，返回 curent_regs，ESP 就恢复到调用 handler 之前...
       
       等待，isr.asm:
       push esp  <-- regs 参数
       call handler
       mov esp, eax
       
       如果 handler 返回的是 regs (即旧 ESP)，那么 esp 恢复原位，接下来的 pop gs 等正常执行。
       
       所以，我们只需要把 current_regs 保存进 PCB 即可。
    */
    current_process->esp = (uint32_t)current_regs;
    
    /* 2. 轮转调度 (Round Robin) */
    /* 循环查找下一个处于 READY 状态的进程 */
    process_t* next = current_process->next;
    while (next->state != STATE_READY) {
        next = next->next;
        /* 安全保障：PID 0 (Kernel_Idle) 永远是 READY 的，
           所以这个循环一定能退出来，不会死循环。 */
    }
    current_process = next;
    
    /* 3. 更新 TSS 中的内核栈 */
    /* 当从这个新任务的 User Mode 发生中断时，CPU 会自动切换到这个 esp0 */
    tss_set_stack(current_process->kernel_stack_top);
    
    /* 4. 返回新任务的栈指针 */
    return (struct registers*)current_process->esp;
}

void process_update_sleep_ticks(void) {
    if (!process_list) return;

    process_t* first = process_list;
    process_t* curr = first;
    
    do {
        if (curr->state == STATE_SLEEPING) {
            if (curr->sleep_ticks > 0) {
                curr->sleep_ticks--;
            }
            if (curr->sleep_ticks == 0) {
                curr->state = STATE_READY;
            }
        }
        curr = curr->next;
    } while (curr != first);
}

void process_sleep(uint32_t ticks) {
    if (current_process && current_process->pid != 0) {
        current_process->state = STATE_SLEEPING;
        current_process->sleep_ticks = ticks;
    }
}
