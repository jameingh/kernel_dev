#include "heap.h"
#include "terminal.h"

static header_t* heap_head = NULL;

void kheap_init(void) {
    /* 堆的起始地址已经是映射好的有效内存 (0xD0000000) */
    heap_head = (header_t*)KHEAP_START;
    
    /* 将整个 1MB 作为一个巨大的空闲块 */
    heap_head->size = KHEAP_INITIAL_SIZE - sizeof(header_t);
    heap_head->next = NULL;
    heap_head->is_free = 1;
    
    terminal_writestring("Heap initialized at 0xD0000000 (1MB)\n");
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    /* 4字节对齐 */
    size_t aligned_size = (size + 3) & ~3;
    
    header_t* curr = heap_head;
    while (curr != NULL) {
        if (curr->is_free && curr->size >= aligned_size) {
            /* 找到合适的块 */
            
            /* 检查是否可以分割（剩余空间能否容纳一个新的 Header + 4字节数据？） */
            if (curr->size >= aligned_size + sizeof(header_t) + 4) {
                /* 分割 */
                header_t* new_block = (header_t*)((uint32_t)curr + sizeof(header_t) + aligned_size);
                
                new_block->size = curr->size - aligned_size - sizeof(header_t);
                new_block->is_free = 1;
                new_block->next = curr->next;
                
                curr->size = aligned_size;
                curr->next = new_block;
            }
            
            curr->is_free = 0;
            /* 返回数据区指针（跳过 Header） */
            return (void*)((uint32_t)curr + sizeof(header_t));
        }
        curr = curr->next;
    }
    
    terminal_writestring("OOM: kmalloc failed!\n");
    return NULL;
}

void kfree(void* ptr) {
    if (ptr == NULL) return;
    
    /* 回退到 Header */
    header_t* header = (header_t*)((uint32_t)ptr - sizeof(header_t));
    header->is_free = 1;
    
    /* 合并 (Coalescing) */
    /* 我们需要再次遍历链表来找到 header 的前驱，或者简单点：只向后合并 */
    /* 简单的向后合并策略：如果 next 也是 free，就吃掉它 */
    if (header->next != NULL && header->next->is_free) {
        header->size += sizeof(header_t) + header->next->size;
        header->next = header->next->next;
    }
}
