# 第10周：虚拟文件系统 (VFS) 理论

> **目标**：给所有数据穿上统一的“外衣”。无论是硬盘上的文件、键盘输入、还是内存中的数据，在操作系统眼里，它们都是 **文件**。

## 1. 为什么需要 VFS (Virtual File System)?

操作系统的世界里有千奇百怪的设备：硬盘 (SATA/NVMe)、光盘 (ISO9660)、网络 (NFS)、甚至终端屏幕。
如果应用程序需要分别针对每种设备写代码（例如 `read_harddisk()`, `read_keyboard()`），那就乱套了。

**VFS** 是内核中的一个**抽象层**，它规定了一套标准接口（API）：`read`, `write`, `open`, `close`。
所有设备驱动必须适配这套接口。这就是 Unix 哲学：**一切皆文件 (Everything is a file)**。

```mermaid
graph TD
    App[用户程序] -->|read/write| VFS["VFS (抽象层)"]
    VFS -->|分发| Ext2["硬盘驱动 (Ext2)"]
    VFS -->|分发| Keyboard["键盘驱动 (TTY)"]
    VFS -->|分发| InitRD["内存盘 (InitRD)"]
    
    Ext2 --> HW1[硬盘硬件]
    Keyboard --> HW2[键盘硬件]
    InitRD --> RAM[内存镜像]
    
    style VFS fill:#fff9c4,stroke:#fbc02d
    style App fill:#bbdefb
```

## 2. 核心数据结构：fs_node (文件节点)

在面向对象的语言（如 C++，Java）中，我们可以用 `Interface` 来实现多态。在 C 语言内核中，我们用 **函数指针结构体** 来实现。

我们定义一个通用的文件节点 `fs_node`，它不关心背后的实现，只包含操作函数指针。

```mermaid
classDiagram
    class fs_node {
        +char name[32]
        +uint32_t inode
        +read_type_t read
        +write_type_t write
        +open_type_t open
        +close_type_t close
        +struct fs_node* ptr (私有数据)
    }
    
    class File_System_Impl {
        <<Interface>>
        +read()
        +write()
    }
    
    fs_node ..> File_System_Impl : 指向具体实现
    note for fs_node "这是 VFS 的核心！</br>VFS 只调用 node->read()</br>具体是读硬盘还是读键盘，</br>取决于谁填充了这个指针。"
```

## 3. 什么是 InitRD (Initial Ramdisk)?

实现真正的硬盘驱动（如 IDE/AHCI）和文件系统（如 FAT32/Ext2）非常复杂。
为了尽快拥有文件系统，我们走一个捷径：**InitRD**。

*   **原理**：在构建操作系统镜像时，把一堆文件（hello.txt, test.bin）打包成一个简单的二进制包。
*   **加载**：GRUB 引导加载器把这个包和内核一起加载到内存中。
*   **使用**：内核启动后，直接把这段内存区域当作一个“只读硬盘”来解析。

它不需要读写 IO 端口，纯粹是内存操作，速度极快，非常适合作为我们的第一个文件系统。

```mermaid
graph LR
    subgraph Build["构建阶段"]
    File1[hello.txt] & File2[test.c] -->|打包| DiskImg[initrd.img]
    end
    
    subgraph Boot["启动阶段"]
    DiskImg -->|GRUB 加载| RAM["内存 (0xA00000)"]
    end
    
    subgraph Kernel["内核运行"]
    RAM -->|解析| RootFS[根文件系统 /]
    end
    
    style Build fill:#e1f5fe
    style Boot fill:#e8f5e9
    style Kernel fill:#fff3e0
```

---

### 下一阶段任务
我们将定义 `fs.h` 中的 `fs_node` 结构体，并实现一个极其简单的 `InitRD` 驱动，让你能通过 `vfs_read` 读取打包进去的文件内容。
