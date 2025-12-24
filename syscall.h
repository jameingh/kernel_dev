#ifndef SYSCALL_H
#define SYSCALL_H

#include "interrupts.h"

void syscall_init(void);
void syscall_handler(struct registers* regs);

#endif
