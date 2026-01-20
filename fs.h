#pragma once
#include <stdint.h>
#include <stddef.h>

/* === VFS 节点类型标志 === */
/* 用于 fs_node_t.flags，标识节点类型 */
#define FS_FILE        0x01  // 普通文件
#define FS_DIRECTORY   0x02  // 目录
#define FS_CHARDEVICE  0x03  // 字符设备 (如键盘、串口)
#define FS_BLOCKDEVICE 0x04  // 块设备 (如硬盘)
#define FS_PIPE        0x05  // 管道 (IPC)
#define FS_SYMLINK     0x06  // 符号链接
#define FS_MOUNTPOINT  0x08  // 挂载点 (指示该目录挂载了其他文件系统)

// 前向声明，解决类型定义循环依赖
struct fs_node;

/* === 文件操作函数指针类型 === */
/* 
 * 这些是 VFS 驱动必须实现的接口。
 * 不同的文件系统（InitRD, FAT32等）将提供不同的实现函数。
 */

/**
 * @brief 读取文件内容
 * @param node   操作的文件节点
 * @param offset 读取起始位置（字节偏移）
 * @param size   请求读取的字节数
 * @param buffer 目标缓冲区指针
 * @return uint32_t 实际读取的字节数
 */
typedef uint32_t (*read_type_t)(struct fs_node* node, uint32_t offset, uint32_t size, uint8_t* buffer);

/**
 * @brief 写入文件内容
 * @param node   操作的文件节点
 * @param offset 写入起始位置
 * @param size   请求写入的字节数
 * @param buffer 源数据缓冲区
 * @return uint32_t 实际写入的字节数
 */
typedef uint32_t (*write_type_t)(struct fs_node* node, uint32_t offset, uint32_t size, uint8_t* buffer);

/**
 * @brief 打开文件/目录时的回调
 * @param node 操作的文件节点
 */
typedef void (*open_type_t)(struct fs_node* node);

/**
 * @brief 关闭文件/目录时的回调
 * @param node 操作的文件节点
 */
typedef void (*close_type_t)(struct fs_node* node);

/**
 * @brief 在目录中查找文件
 * @param node 目录节点
 * @param name 目标文件名
 * @return struct fs_node* 找到的文件节点，失败返回 NULL
 */
typedef struct fs_node* (*finddir_type_t)(struct fs_node* node, char* name);

/* === 文件节点结构体 (VFS Core) === */
/* 
 * 这是 VFS 的核心结构。
 * 无论底层是 RAMDisk、硬盘还是网络文件系统，
 * 在内核看来都只是一个统一的 fs_node_t。
 */
typedef struct fs_node {
    char name[32];          /* 文件名 (最大32字符) */
    uint32_t flags;         /* 类型标志 (FS_FILE, FS_DIRECTORY 等) */
    uint32_t inode;         /* 索引节点号 (文件系统内唯一标识) */
    uint32_t length;        /* 文件大小 (字节) */
    
    /* 操作函数表 (类似于 C++ 的虚函数表) */
    read_type_t read;       /* 读函数指针 */
    write_type_t write;     /* 写函数指针 */
    open_type_t open;       /* 打开函数指针 */
    close_type_t close;     /* 关闭函数指针 */
    finddir_type_t finddir; /* 查找目录函数指针 (仅目录有效) */
    
    /* 
     * 驱动私有数据指针 (Hook)
     * 用于底层驱动存储该节点特有的数据结构（如 initrd_file_header_t*）。
     * VFS 层不应直接访问此字段的内容。
     */
    struct fs_node* ptr;    
} fs_node_t;

/* === 标准 VFS 接口封装 === */
/* 
 * 内核其他部分应调用这些函数，而不是直接调用 node->read(...)
 * 这些函数会处理空指针检查等通用逻辑。
 */

/**
 * @brief 封装后的文件读取接口
 */
uint32_t vfs_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);

/**
 * @brief 封装后的文件打开接口
 */
void vfs_open(fs_node_t* node, uint8_t read, uint8_t write);

/**
 * @brief 封装后的目录查找接口
 */
fs_node_t* vfs_finddir(fs_node_t* node, char* name);

/* 全局根节点 (/)，文件系统树的起点 */
extern fs_node_t* fs_root;
