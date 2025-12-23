#pragma once
#include "interrupts.h"
#include <stdint.h>

#define PROCESS_NAME_LEN 32

typedef struct process {
    uint32_t pid;
    uint32_t esp;       /* 内核栈指针 (Saved ESP) */
    char name[PROCESS_NAME_LEN];
    struct process* next;
} process_t;

/* 初始化多任务系统 (将当前流作为 Idle 任务) */
void process_init(void);

/* 创建新内核线程 */
process_t* process_create(void (*entry_point)(void), const char* name);

/* 调度函数 (被时钟中断调用) */
struct registers* schedule(struct registers* current_regs);
