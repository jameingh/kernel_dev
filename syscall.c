#include "syscall.h"
#include "terminal.h"
#include "process.h"

static void sys_write(char* str) {
    terminal_writestring(str);
}

struct registers* syscall_handler(struct registers* regs) {
    // 简单的系统调用分发
    // EAX = 系统调用号
    // EBX = 参数 1
    if (regs->eax == 1) { // 约定 1 为 write
        sys_write((char*)regs->ebx);
    } else if (regs->eax == 2) { // 约定 2 为 yield
        return schedule(regs);
    }
    return regs;
}
