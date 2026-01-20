#include "heap.h"
#include "terminal.h"

/* 堆的链表头指针，指向第一个内存块的 Header */
static header_t* heap_head = NULL;

/**
 * @brief 初始化内核堆
 * 
 * 假设：0xD0000000 这个虚拟地址对应的物理页已经由 VMM 分配并映射好了。
 * 行为：在堆的起始位置建立第一个 Header，管理整个 1MB 空间。
 */
void kheap_init(void) {
    /* 堆的起始地址已经是映射好的有效内存 (0xD0000000) */
    heap_head = (header_t*)KHEAP_START;
    
    /* 将整个 1MB 作为一个巨大的空闲块
       可用大小 = 总大小 - Header本身的大小 */
    heap_head->size = KHEAP_INITIAL_SIZE - sizeof(header_t);
    heap_head->next = NULL;
    heap_head->is_free = 1;
    
    terminal_writestring("Heap initialized at 0xD0000000 (1MB)\n");
}

/**
 * @brief 内核内存分配 (First Fit 算法)
 * 
 * 1. 遍历链表，寻找第一个足够大的空闲块 (First Fit)。
 * 2. 如果找到的块比请求的大很多，则将其分割 (Split) 成两部分：
 *    - 第一部分：大小刚好满足请求，标记为已分配。
 *    - 第二部分：剩余空间，成为一个新的空闲块。
 * 3. 如果没找到，返回 NULL (Out of Memory)。
 * 
 * @see [heap_allocation.md](doc/heap_allocation.md)
 * @param size 请求分配的字节数
 * @return void* 指向数据区的指针 (跳过 Header)
 */
void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    /* 内存对齐：将请求大小向上对齐到 4 字节
       例如：申请 5 字节 -> 实际分配 8 字节
       公式：(size + 3) & ~3 等价于 ceil(size/4) * 4 */
    size_t aligned_size = (size + 3) & ~3;
    
    header_t* curr = heap_head;
    
    // 遍历链表寻找空闲块
    while (curr != NULL) {
        if (curr->is_free && curr->size >= aligned_size) {
            /* === 找到合适的块 (First Fit) === */
            
            /* 检查是否可以分割？
               条件：当前块大小 >= 请求大小 + 一个新Header的大小 + 至少4字节的数据区
               如果不满足，说明剩余空间太小，不值得分割，直接把整个块都给它。 */
            if (curr->size >= aligned_size + sizeof(header_t) + 4) {
                /* === 执行分割 (Splitting) === */
                
                // 计算新块（剩余空闲块）的起始地址
                // 地址 = 当前块头 + Header大小 + 请求的数据区大小
                header_t* new_block = (header_t*)((uint32_t)curr + sizeof(header_t) + aligned_size);
                
                // 设置新块的属性
                new_block->size = curr->size - aligned_size - sizeof(header_t);
                new_block->is_free = 1;
                new_block->next = curr->next;
                
                // 更新当前块的属性
                curr->size = aligned_size;
                curr->next = new_block;
            }
            
            // 标记为已分配
            curr->is_free = 0;
            
            /* 返回数据区指针（跳过 Header） */
            return (void*)((uint32_t)curr + sizeof(header_t));
        }
        curr = curr->next;
    }
    
    terminal_writestring("OOM: kmalloc failed!\n");
    return NULL;
}

/**
 * @brief 释放内核内存
 * 
 * 1. 根据指针回退找到 Header。
 * 2. 标记为空闲。
 * 3. (简单合并) 检查下一个块是否也是空闲的，如果是，合并它们。
 * 
 * @param ptr 要释放的内存指针
 */
void kfree(void* ptr) {
    if (ptr == NULL) return;
    
    /* 回退到 Header：用户拿到的是数据区指针，Header 在它前面 */
    header_t* header = (header_t*)((uint32_t)ptr - sizeof(header_t));
    header->is_free = 1;
    
    /* === 内存合并 (Coalescing) === */
    /* 这里实现的是简单的向后合并：
       如果当前块释放后，发现它的 next 块也是空闲的，就把它们融合成一个大块。
       这样可以减少碎片。
       
       注意：完整的实现还应该检查“前一个”块是否空闲并进行向前合并，
       但这需要双向链表或更复杂的逻辑。当前单向链表只能方便地向后合并。 */
    if (header->next != NULL && header->next->is_free) {
        // 新大小 = 当前大小 + 下一个Header大小 + 下一个数据区大小
        header->size += sizeof(header_t) + header->next->size;
        
        // 删除下一个节点
        header->next = header->next->next;
    }
}
