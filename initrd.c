#include "fs.h"
#include "heap.h"
#include "terminal.h"

// 全局文件系统根节点
fs_node_t* fs_root = NULL;

/* === VFS Implementation (虚拟文件系统实现) === */
/* VFS 是一个抽象层，它定义了统一的文件操作接口，
   并将具体操作转发给底层的具体文件系统（如 InitRD, FAT32 等）。 */

/**
 * @brief VFS 读取文件
 * 
 * @param node   要读取的文件节点
 * @param offset 读取的起始偏移量
 * @param size   要读取的字节数
 * @param buffer 目标缓冲区
 * @return uint32_t 实际读取的字节数
 */
uint32_t vfs_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    // 检查节点是否实现了 read 函数指针
    if (node->read != NULL) {
        return node->read(node, offset, size, buffer);
    } else {
        return 0;
    }
}

/**
 * @brief VFS 打开文件
 * 
 * @param node  要打开的文件节点
 * @param read  是否以读模式打开
 * @param write 是否以写模式打开
 */
void vfs_open(fs_node_t* node, uint8_t read, uint8_t write) {
    if (node->open != NULL)
        node->open(node);
}

/**
 * @brief VFS 查找目录中的文件
 * 
 * @param node 目录节点
 * @param name 要查找的文件名
 * @return fs_node_t* 找到的文件节点，未找到则返回 NULL
 */
fs_node_t* vfs_finddir(fs_node_t* node, char* name) {
    // 检查节点是否为目录且实现了 finddir 函数
    if ((node->flags & 0x7) == FS_DIRECTORY && node->finddir != NULL)
        return node->finddir(node, name);
    else
        return NULL;
}

/* === InitRD Implementation (Simulated) === */
/* InitRD (Initial Ramdisk) 是一个极其简单的只读文件系统，
   它在内存中模拟一个磁盘镜像。在本实现中，我们手动构建了一个
   包含 "hello.txt" 的伪造磁盘镜像。 */

/* InitRD 头部结构 */
typedef struct {
    uint32_t nfiles; /* 文件数量 */
} initrd_header_t;

/* InitRD 文件头结构 - 描述每个文件的元数据 */
typedef struct {
    uint8_t magic;    /* 魔数 (0xBF)用于校验文件头是否合法 */
    char name[32];    /* 文件名 */
    uint32_t offset;  /* 数据在 InitRD 镜像中的偏移量 */
    uint32_t length;  /* 文件长度 (字节) */
} initrd_file_header_t;

/* 
 * 伪造的磁盘镜像数据。
 * 使用 static 数组模拟一段连续的内存作为 "磁盘"。
 * {1} 初始化是为了强制将其放入 .data 段而不是 .bss 段，
 * 确保它在二进制文件中占有实际空间（虽然对于运行时构建来说 .bss 也可以）。
 */
static uint8_t fake_disk[1024] = {1}; // Force .data section

static fs_node_t* initrd_root;       // InitRD 的根目录节点
static fs_node_t* initrd_dev_nodes;  // 存储所有文件节点的数组
static int n_root_nodes;             // 根目录下的文件数量

/* 字符串比较辅助函数 */
static int strcmp(const char* s1, const char* s2) {
    while(*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/**
 * @brief InitRD 读操作的具体实现
 * 
 * @param node   文件对应的 VFS 节点
 * @param offset 偏移量
 * @param size   读取大小
 * @param buffer 输出缓冲区
 * @return uint32_t 实际读取大小
 */
uint32_t initrd_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    // 检查内部指针有效性
    if (node->ptr == NULL) return 0;
    
    /* 获取文件头信息。
       node->ptr 在初始化时被指向了 fake_disk 中的 initrd_file_header_t */
    initrd_file_header_t* header = (initrd_file_header_t*)node->ptr;
    
    // 边界检查
    if (offset > header->length) return 0;
    if (offset + size > header->length) size = header->length - offset;
    
    /* 计算数据在 fake_disk 中的绝对地址
       数据位置 = fake_disk起始地址 + 文件偏移量 */
    uint8_t* data_ptr = (uint8_t*)((uint32_t)fake_disk + header->offset);
    
    // 拷贝数据到用户缓冲区
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = data_ptr[offset + i];
    }
    
    return size;
}

/**
 * @brief InitRD 目录查找的具体实现
 * 
 * @param node 目录节点
 * @param name 文件名
 * @return fs_node_t* 找到的文件节点
 */
fs_node_t* initrd_finddir(fs_node_t* node, char* name) {
    /* InitRD 是扁平结构，所有文件都在根目录下。
       我们只需遍历初始化时创建的节点数组。 */
    for (int i = 0; i < n_root_nodes; i++) {
        if (strcmp(name, initrd_dev_nodes[i].name) == 0) {
            return &initrd_dev_nodes[i];
        }
    }
    return NULL;
}

/**
 * @brief 构建伪造的磁盘镜像
 * 
 * 这个函数手动填充 fake_disk 数组，模拟一个包含 "hello.txt" 的文件系统。
 * 结构如下：
 * [ 4字节: 文件数(1) ]
 * [ 文件头(initrd_file_header_t) ]
 * [ 文件内容("Hello VFS World!") ]
 */
void initrd_build_fake_disk(void) {
    /* 1. 写入文件数量 (int 1) */
    uint32_t* nfiles_ptr = (uint32_t*)fake_disk;
    *nfiles_ptr = 1;
    
    /* 2. 写入文件头 */
    /* Offset 4: 跳过文件数量占用的4字节 */
    initrd_file_header_t* file1 = (initrd_file_header_t*)(fake_disk + 4);
    file1->magic = 0xBF; // 写入魔数
    
    /* 写入文件名 "hello.txt" */
    char* fname = "hello.txt";
    for(int i=0; i<32; i++) file1->name[i] = (i < 9) ? fname[i] : 0;
    
    file1->length = 17; // "Hello VFS World!" + null terminator
    file1->offset = 4 + sizeof(initrd_file_header_t); // 数据紧跟在文件头后面
    
    /* 3. 写入文件内容 */
    char* content = "Hello VFS World!";
    uint8_t* data = fake_disk + file1->offset;
    for(int i=0; i<17; i++) data[i] = content[i];
}

/**
 * @brief 初始化 InitRD 系统
 * 
 * @return fs_node_t* 返回根目录节点
 */
fs_node_t* initrd_init(void) {
    terminal_writestring("Building fake initrd image...\n");
    
    // 1. 构建内存中的磁盘镜像
    initrd_build_fake_disk();
    
    /* 2. 解析 fake_disk 头部信息 */
    uint32_t nfiles = *(uint32_t*)fake_disk;
    n_root_nodes = nfiles;
    
    /* 3. 为所有文件分配 VFS 节点数组 */
    initrd_dev_nodes = (fs_node_t*)kmalloc(sizeof(fs_node_t) * nfiles);
    
    uint32_t offset = 4; // 跳过 nfiles
    for (int i = 0; i < nfiles; i++) {
        // 获取当前文件的头部指针
        initrd_file_header_t* header = (initrd_file_header_t*)(fake_disk + offset);
        
        /* 填充 VFS 节点信息 */
        fs_node_t* node = &initrd_dev_nodes[i];
        
        // 复制文件名
        for(int j=0; j<32; j++) node->name[j] = header->name[j];
        
        node->flags = FS_FILE;       // 标记为普通文件
        node->length = header->length;
        node->inode = i;             // 索引节点号
        node->read = &initrd_read;   // 绑定读函数
        node->write = NULL;          // 不支持写
        node->open = NULL;
        node->close = NULL;
        node->ptr = (struct fs_node*)header; // 关键：将 ptr 指回原始头部，供 read 函数使用
        
        // 移动到下一个文件头（如果有）
        // 注意：在标准 InitRD 中，头部通常集中在前面，这里是简化的紧凑布局
        // 实际上我们的 fake_disk 布局是: [Count] [Header1] [Data1] [Header2] [Data2] ...
        // 所以 offset 需要跳过 Header + Data
        offset += sizeof(initrd_file_header_t) + header->length;
    }
    
    /* 4. 创建并初始化根目录节点 */
    initrd_root = (fs_node_t*)kmalloc(sizeof(fs_node_t));
    char* rootname = "initrd";
    for(int i=0; i<7; i++) initrd_root->name[i] = rootname[i];
    
    initrd_root->flags = FS_DIRECTORY;
    initrd_root->finddir = &initrd_finddir; // 绑定目录查找函数
    initrd_root->read = NULL;
    initrd_root->ptr = NULL;
    
    return initrd_root;
}
