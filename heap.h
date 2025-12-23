#pragma once
#include <stdint.h>
#include <stddef.h>

#define KHEAP_START     0xD0000000
#define KHEAP_INITIAL_SIZE  (1024 * 1024) /* 1MB */

/* 内存块元数据头 */
typedef struct header {
    struct header* next;
    size_t size; /* 数据区大小，不包含 Header 本身 */
    uint8_t is_free;
    uint8_t padding[3]; /* 对齐到 4/8 字节 */
} header_t;

void kheap_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
