#pragma once 
// #pragma once 是一个 预处理指令 ，它的作用是 保证当前头文件在同一个编译单元中只被包含（include）一次 。
#include "fs.h"

/**
 * @brief 初始化初始内存盘 (InitRD)
 * 
 * 该函数负责构建和解析一个简单的内存文件系统。
 * 具体步骤包括：
 * 1. 构建一个伪造的磁盘镜像 (fake_disk)，包含预设的文件数据。
 * 2. 解析该镜像的头部信息，提取文件列表。
 * 3. 为每个文件创建 VFS 节点 (fs_node_t)，并挂载到文件系统树中。
 * 4. 创建并返回根目录节点。
 * 
 * @return fs_node_t* 指向 InitRD 根目录节点的指针。
 */
fs_node_t* initrd_init(void);
