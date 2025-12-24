#pragma once
#include <stdint.h>
#include <stddef.h>

#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x03
#define FS_BLOCKDEVICE 0x04
#define FS_PIPE        0x05
#define FS_SYMLINK     0x06
#define FS_MOUNTPOINT  0x08

struct fs_node;

/* 函数指针类型定义 */
typedef uint32_t (*read_type_t)(struct fs_node*, uint32_t, uint32_t, uint8_t*);
typedef uint32_t (*write_type_t)(struct fs_node*, uint32_t, uint32_t, uint8_t*);
typedef void (*open_type_t)(struct fs_node*);
typedef void (*close_type_t)(struct fs_node*);
typedef struct fs_node* (*finddir_type_t)(struct fs_node*, char* name);

/* 文件节点结构体 (VFS Core) */
typedef struct fs_node {
    char name[32];          /* 文件名 */
    uint32_t flags;         /* 类型标志 (FILE, DIRECTORY...) */
    uint32_t inode;         /* 索引节点号 */
    uint32_t length;        /* 文件大小 */
    read_type_t read;       /* 读函数指针 */
    write_type_t write;     /* 写函数指针 */
    open_type_t open;       /* 打开函数指针 */
    close_type_t close;     /* 关闭函数指针 */
    finddir_type_t finddir; /* 查找目录函数指针 */
    struct fs_node* ptr;    /* 驱动私有数据 (hook) */
} fs_node_t;

/* 标准 VFS 接口封装 */
uint32_t vfs_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
void vfs_open(fs_node_t* node, uint8_t read, uint8_t write);
fs_node_t* vfs_finddir(fs_node_t* node, char* name);

/* 全局根节点 */
extern fs_node_t* fs_root;
