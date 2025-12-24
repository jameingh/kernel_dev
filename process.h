#pragma once
#include "interrupts.h"
#include <stdint.h>

#define PROCESS_NAME_LEN 32

typedef struct process {
    uint32_t pid;
    uint32_t esp;              /* 当前保存的栈指针 (struct registers*) */
    uint32_t kernel_stack_top; /* 初始/基础内核栈顶 (用于 TSS.esp0) */
    char name[PROCESS_NAME_LEN];
    struct process* next;
} process_t;

/* 初始化多任务系统 (将当前流作为 Idle 任务) */
void process_init(void);

/* 创建新内核线程 (Ring 0) */
process_t* process_create(void (*entry_point)(void), const char* name);

/* 创建新用户进程 (Ring 3) */
process_t* process_create_user(void (*entry_point)(void), const char* name);

/* 调度函数 (被时钟中断调用) */
struct registers* schedule(struct registers* current_regs);
