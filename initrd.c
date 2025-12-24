#include "fs.h"
#include "heap.h"
#include "terminal.h"

fs_node_t* fs_root = NULL;

/* === VFS Implementation === */

uint32_t vfs_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    if (node->read != NULL) {
        return node->read(node, offset, size, buffer);
    } else {
        return 0;
    }
}

void vfs_open(fs_node_t* node, uint8_t read, uint8_t write) {
    if (node->open != NULL)
        node->open(node);
}

fs_node_t* vfs_finddir(fs_node_t* node, char* name) {
    if ((node->flags & 0x7) == FS_DIRECTORY && node->finddir != NULL)
        return node->finddir(node, name);
    else
        return NULL;
}

/* === InitRD Implementation (Simulated) === */

/* InitRD 头部结构 */
typedef struct {
    uint32_t nfiles; /* 文件数量 */
} initrd_header_t;

typedef struct {
    uint8_t magic;    /* 魔数 (0xBF)用于校验 */
    char name[32];    /* 文件名 */
    uint32_t offset;  /* 数据在 InitRD 中的偏移 */
    uint32_t length;  /* 文件长度 */
} initrd_file_header_t;

static uint8_t fake_disk[1024] = {1}; // Force .data section

static fs_node_t* initrd_root;
static fs_node_t* initrd_dev_nodes; // Array of nodes
static int n_root_nodes;

/* 字符串比较辅助函数 */
static int strcmp(const char* s1, const char* s2) {
    while(*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/* InitRD 读操作实现 */
uint32_t initrd_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    // Check ptr validity
    if (node->ptr == NULL) return 0;
    
    /* 改为使用指针，避免结构体拷贝导致的潜在栈/对齐问题 */
    initrd_file_header_t* header = (initrd_file_header_t*)node->ptr;
    
    if (offset > header->length) return 0;
    if (offset + size > header->length) size = header->length - offset;
    
    /* 数据其实就在 fake_disk 里 */
    uint8_t* data_ptr = (uint8_t*)((uint32_t)fake_disk + header->offset);
    
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = data_ptr[offset + i];
    }
    
    return size;
}

/* InitRD 目录查找实现 */
fs_node_t* initrd_finddir(fs_node_t* node, char* name) {
    /* 遍历所有子节点 - 这里我们知道这是一个扁平结构 */
    /* 实际上，我们在 init 时已经转换成了 fs_node 数组 */
    for (int i = 0; i < n_root_nodes; i++) {
        if (strcmp(name, initrd_dev_nodes[i].name) == 0) {
            return &initrd_dev_nodes[i];
        }
    }
    return NULL;
}

void initrd_build_fake_disk(void) {
    /* 1. 写入文件数量 (int 1) */
    uint32_t* nfiles_ptr = (uint32_t*)fake_disk;
    *nfiles_ptr = 1;
    
    /* 2. 写入文件头 */
    /* Offset 4 */
    initrd_file_header_t* file1 = (initrd_file_header_t*)(fake_disk + 4);
    file1->magic = 0xBF;
    
    /* strcpy "hello.txt" */
    char* fname = "hello.txt";
    for(int i=0; i<32; i++) file1->name[i] = (i < 9) ? fname[i] : 0;
    
    file1->length = 17; // "Hello VFS World!" + null
    file1->offset = 4 + sizeof(initrd_file_header_t); // 数据紧跟在头后面
    
    /* 3. 写入文件内容 */
    char* content = "Hello VFS World!";
    uint8_t* data = fake_disk + file1->offset;
    for(int i=0; i<17; i++) data[i] = content[i];
}

fs_node_t* initrd_init(void) {
    terminal_writestring("Building fake initrd image...\n");
    initrd_build_fake_disk();
    
    /* 解析 fake_disk */
    uint32_t nfiles = *(uint32_t*)fake_disk;
    n_root_nodes = nfiles;
    
    /* 分配节点数组 */
    initrd_dev_nodes = (fs_node_t*)kmalloc(sizeof(fs_node_t) * nfiles);
    
    uint32_t offset = 4;
    for (int i = 0; i < nfiles; i++) {
        initrd_file_header_t* header = (initrd_file_header_t*)(fake_disk + offset);
        
        /* 填充 VFS 节点 */
        fs_node_t* node = &initrd_dev_nodes[i];
        
        // strcpy name
        for(int j=0; j<32; j++) node->name[j] = header->name[j];
        
        node->flags = FS_FILE;
        node->length = header->length;
        node->inode = i;
        node->read = &initrd_read;
        node->write = NULL;
        node->open = NULL;
        node->close = NULL;
        node->ptr = (struct fs_node*)header; // 指回原始头部以便 read 时用
        
        offset += sizeof(initrd_file_header_t) + header->length;
    }
    
    /* 创建根节点 */
    initrd_root = (fs_node_t*)kmalloc(sizeof(fs_node_t));
    char* rootname = "initrd";
    for(int i=0; i<7; i++) initrd_root->name[i] = rootname[i];
    
    initrd_root->flags = FS_DIRECTORY;
    initrd_root->finddir = &initrd_finddir;
    initrd_root->read = NULL;
    initrd_root->ptr = NULL;
    
    return initrd_root;
}
