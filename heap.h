#pragma once
#include <stdint.h>
#include <stddef.h>

/* === 内核堆配置 === */
/* 
 * KHEAP_START: 内核堆的虚拟起始地址。
 * 注意：这个地址必须在页表中被映射并标记为可写。
 * 在当前的 vmm.c 实现中，我们预留了这一块区域。
 */
#define KHEAP_START     0xD0000000

/* KHEAP_INITIAL_SIZE: 初始堆大小 (1MB) */
#define KHEAP_INITIAL_SIZE  (1024 * 1024) 

/* 
 * === 内存块元数据头 (Block Header) ===
 * 堆内存被组织成一个单向链表。每个分配出去的（或空闲的）内存块
 * 之前都有这样一个头部，用来记录块的大小和状态。
 */
typedef struct header {
    struct header* next;  /* 指向链表中下一个内存块的头部 */
    size_t size;          /* 当前块的数据区大小（不包含 Header 本身的大小） */
    uint8_t is_free;      /* 标志位：1 表示空闲，0 表示已分配 */
    uint8_t padding[3];   /* 填充字节，确保 Header 结构体大小对齐到 4 或 8 字节 */
} header_t;

/**
 * @brief 初始化内核堆管理器
 * 
 * 设置堆的起始状态，创建一个覆盖整个堆区域的巨大空闲块。
 * 注意：必须在 VMM (虚拟内存管理) 初始化之后调用。
 */
void kheap_init(void);

/**
 * @brief 申请内核内存
 * 
 * 类似于标准库的 malloc。从堆中寻找足够大的空闲块并分配。
 * 
 * @param size 需要分配的字节数
 * @return void* 指向分配到的内存区域的指针（不包含 Header）。如果分配失败返回 NULL。
 */
void* kmalloc(size_t size);

/**
 * @brief 释放内核内存
 * 
 * 类似于标准库的 free。将内存块标记为空闲，并尝试与后一个空闲块合并。
 * 
 * @param ptr kmalloc 返回的指针
 */
void kfree(void* ptr);
