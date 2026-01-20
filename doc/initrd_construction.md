# InitRD 伪造磁盘构建过程图解

该文档详细解释了 `initrd.c` 中 `initrd_build_fake_disk` 函数的执行过程及其对内存布局的影响。

## 1. 函数功能概述

`initrd_build_fake_disk` 的作用是在内存数组 `fake_disk` 中手动构建一个简单的文件系统镜像。这个镜像包含：
1.  **超级块 (Superblock)**：这里简化为仅包含一个“文件数量”字段。
2.  **文件头 (File Headers)**：描述每个文件的元数据（文件名、位置、大小）。
3.  **文件内容 (File Content)**：实际的文件数据。

## 2. 执行步骤图解

### 步骤 1: 初始化文件数量

```c
uint32_t* nfiles_ptr = (uint32_t*)fake_disk;
*nfiles_ptr = 1;
```

此时 `fake_disk` 的前 4 个字节被设置为 1。

### 步骤 2: 写入文件头 (Header)

```c
initrd_file_header_t* file1 = (initrd_file_header_t*)(fake_disk + 4);
file1->magic = 0xBF;
// ... 复制文件名 "hello.txt" ...
file1->length = 17;
file1->offset = 4 + sizeof(initrd_file_header_t); 
```

我们在偏移量 4（跳过前面的 `nfiles`）处构建文件头结构体。关键是计算 `offset`，它指向紧随其后的数据区。

### 步骤 3: 写入文件内容

```c
char* content = "Hello VFS World!";
uint8_t* data = fake_disk + file1->offset;
// ... 复制内容 ...
```

将字符串数据写入到 `offset` 指向的内存位置。

## 3. 最终内存布局示意图

下图展示了 `fake_disk` 数组在函数执行完毕后的逻辑视图。

```mermaid
graph TD
    subgraph FakeDisk["fake_disk 内存数组 (Base Address)"]
        direction TB
        
        %% 0x00 - 0x03
        NFiles["Offset 0x00: nfiles (4 bytes)<br/>Value: 1"]
        
        %% 0x04 - 0x2F (假设 Header 大小为 44 字节)
        subgraph Header["Offset 0x04: 文件头 (initrd_file_header_t)"]
            direction TB
            Magic["magic (1 byte): 0xBF"]
            Name["name (32 bytes): 'hello.txt'"]
            Padding["(可能存在的内存对齐填充)"]
            OffsetField["offset (4 bytes): 指向数据区"]
            LengthField["length (4 bytes): 17"]
        end
        
        %% 0x30 - ...
        subgraph Data["文件数据区 (Data Area)"]
            Content["Offset: 4 + sizeof(Header)<br/><br/>'Hello VFS World!'<br/>(17 bytes + null terminator)"]
        end
        
        %% 连接关系
        NFiles --> Header
        Header --> Data
        OffsetField -.->|指针引用| Data
    end
    
    style NFiles fill:#e1f5fe,stroke:#01579b
    style Header fill:#fff9c4,stroke:#fbc02d
    style Data fill:#c8e6c9,stroke:#2e7d32
```

## 4. 关键数据结构参考

```c
typedef struct {
    uint8_t magic;    /* 魔数，用于校验 */
    char name[32];    /* 文件名 */
    uint32_t offset;  /* 数据相对于 fake_disk 起始位置的偏移量 */
    uint32_t length;  /* 文件数据长度 */
} initrd_file_header_t;
```

通过这种方式，`initrd_init` 后续可以通过读取 `fake_disk` 的前 4 个字节知道有多少个文件，然后遍历 Header 数组来挂载 VFS 节点。
