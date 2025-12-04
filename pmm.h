#pragma once
#include <stdint.h>

#define PMM_PAGE_SIZE 4096
#define PMM_TOTAL_MEM_BYTES (64 * 1024 * 1024) /* 可按需调整或后续接入E820 */

void pmm_init(void);
uint32_t pmm_total_pages(void);
uint32_t pmm_free_pages(void);
uint32_t pmm_alloc_page(void);
void pmm_free_page(uint32_t phys_addr);
uint32_t pmm_alloc_contiguous(uint32_t n_pages);

