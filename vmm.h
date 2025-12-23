#pragma once
#include <stdint.h>

/* 页目录项/页表项标志位 */
#define PAGE_PRESENT    0x1
#define PAGE_RW         0x2
#define PAGE_USER       0x4
#define PAGE_FRAME      0xFFFFF000

/* 分页相关常量 */
#define PAGE_SIZE       4096
#define PAGING_FLAG     0x80000000 /* CR0 最高位 */

/* 初始化 VMM，建立恒等映射与高半核映射 */
void vmm_init(void);
