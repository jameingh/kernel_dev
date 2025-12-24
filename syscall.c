#include "syscall.h"
#include "terminal.h"

static void sys_write(char* str) {
    terminal_writestring(str);
}

void syscall_handler(struct registers* regs) {
    // 简单的系统调用分发
    // EAX = 系统调用号
    // EBX = 参数 1
    if (regs->eax == 1) { // 约定 1 为 write
        sys_write((char*)regs->ebx);
    }
}
