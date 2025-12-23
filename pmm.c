#include "pmm.h"
#include "terminal.h"

extern uint32_t _kernel_start; /* 来自链接脚本 */
extern uint32_t _kernel_end;   /* 来自链接脚本 */

static uint32_t total_pages;
static uint32_t free_pages;
static uint8_t bitmap[(PMM_TOTAL_MEM_BYTES / PMM_PAGE_SIZE + 7) / 8];

static inline void bm_set(uint32_t pfn) { bitmap[pfn >> 3] |= (uint8_t)(1u << (pfn & 7)); }
static inline void bm_clear(uint32_t pfn) { bitmap[pfn >> 3] &= (uint8_t)~(1u << (pfn & 7)); }
static inline uint8_t bm_test(uint32_t pfn) { return (bitmap[pfn >> 3] >> (pfn & 7)) & 1u; }

static void reserve_range(uint32_t start_addr, uint32_t end_addr) {
    if (end_addr <= start_addr) return;
    uint32_t start_pfn = start_addr / PMM_PAGE_SIZE;
    uint32_t end_pfn = (end_addr + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
    if (end_pfn > total_pages) end_pfn = total_pages;
    for (uint32_t p = start_pfn; p < end_pfn; ++p) {
        if (bm_test(p)) { bm_clear(p); if (free_pages) free_pages--; }
    }
}

void pmm_init(void) {
    total_pages = PMM_TOTAL_MEM_BYTES / PMM_PAGE_SIZE;
    free_pages = 0;
    /* 初始标记全部可用 */
    for (uint32_t p = 0; p < total_pages; ++p) { bm_set(p); }
    free_pages = total_pages;

    /* 保留低端传统区与视频内存 */
    reserve_range(0x00000000, 0x000A0000); /* 低端传统区 */
    reserve_range(0x000A0000, 0x000C0000); /* 视频内存（含VGA） */
    reserve_range(0x000C0000, 0x00100000); /* BIOS 扩展与系统 BIOS (C0000-FFFFF) */

    /* 保留内核镜像范围（含 .text/.data/.bss） */
    uint32_t kstart = (uint32_t)&_kernel_start;
    uint32_t kend   = (uint32_t)&_kernel_end;
    reserve_range(kstart, kend);

    /* 统计输出 */
    terminal_writestring("PMM initialized\n");
    terminal_writestring("Total pages: ");
    /* 简易十进制输出 */
    uint32_t t = total_pages; char buf[12]; int i = 0; do { buf[i++] = '0' + (t % 10); t /= 10; } while (t); while (i--) terminal_putchar(buf[i]);
    terminal_writestring("\nFree pages: ");
    t = free_pages; i = 0; do { buf[i++] = '0' + (t % 10); t /= 10; } while (t); while (i--) terminal_putchar(buf[i]);
    terminal_putchar('\n');
}

uint32_t pmm_total_pages(void) { return total_pages; }
uint32_t pmm_free_pages(void) { return free_pages; }

uint32_t pmm_alloc_page(void) {
    for (uint32_t p = 0; p < total_pages; ++p) {
        if (bm_test(p)) {
            bm_clear(p);
            if (free_pages) free_pages--;
            return p * PMM_PAGE_SIZE;
        }
    }
    return 0;
}

void pmm_free_page(uint32_t phys_addr) {
    if (phys_addr % PMM_PAGE_SIZE) return; /* 非对齐忽略 */
    uint32_t p = phys_addr / PMM_PAGE_SIZE;
    if (p >= total_pages) return;
    if (!bm_test(p)) { bm_set(p); free_pages++; }
}

uint32_t pmm_alloc_contiguous(uint32_t n_pages) {
    if (n_pages == 0) return 0;
    uint32_t run = 0, start = 0;
    for (uint32_t p = 0; p < total_pages; ++p) {
        if (bm_test(p)) {
            if (run == 0) start = p;
            run++;
            if (run == n_pages) {
                for (uint32_t q = start; q < start + n_pages; ++q) { bm_clear(q); }
                if (free_pages >= n_pages) free_pages -= n_pages;
                return start * PMM_PAGE_SIZE;
            }
        } else {
            run = 0;
        }
    }
    return 0;
}

