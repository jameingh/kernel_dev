# Week 10 实战复盘：给内核装上“硬盘”

> **摘要**：本周我们为内核实现了第一个只读文件系统。包括 VFS 抽象层的定义、一个模拟的内存盘驱动 (InitRD)，以及修复了一个严重的内存访问 Bug。

## 1. VFS 抽象层 (`fs.h`)

这是所有文件系统的基石。我们定义了 `fs_node` 结构体，利用 C 语言的 **函数指针** 实现了类似面向对象的多态性。

```c
typedef struct fs_node {
    char name[32];
    uint32_t flags;
    /* 核心：操作函数指针 */
    read_type_t read;       // -> 指向具体的驱动函数
    write_type_t write;
    open_type_t open;
    close_type_t close;
    finddir_type_t finddir;
    struct fs_node* ptr;    // -> 驱动私有数据
} fs_node_t;
```

## 2. 模拟内存盘驱动 (`initrd.c`)

为了避开复杂的硬盘 IO，我们选择在内存中“伪造”一个硬盘镜像。

### 核心实现
我们定义了一个 1KB 的 `fake_disk` 数组，并强制将其放入 `.data` 段以确保被加载。

```c
/* 强制初始化以确保位于 .data 段，而非 .bss */
static uint8_t fake_disk[1024] = {1}; 

/* 驱动读函数 */
uint32_t initrd_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    // 1. 获取文件头
    // 2. 计算数据在 fake_disk 中的位置
    // 3. memcpy 到 buffer
}
```

### 🐛 踩坑与修复：结构体拷贝崩溃

在实现 `initrd_read` 时，我们遭遇了一个严重的 **Triple Fault (重启循环)**。
原因是直接将 `node->ptr` 指向的数据拷贝到栈上的结构体变量中，引发了未定义行为（可能是对齐问题或栈越界）。

**修复方案 (Use Diff)**：改用指针直接访问，避免内存拷贝。

```diff
 uint32_t initrd_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
-    /* [崩溃版] 尝试把数据拷贝到栈上 */
-    initrd_file_header_t header = *(initrd_file_header_t*)node->ptr;
-
-    if (offset > header.length) return 0;
+    /* [修复版] 直接使用指针访问，零拷贝，安全高效 */
+    initrd_file_header_t* header = (initrd_file_header_t*)node->ptr;
+
+    if (offset > header->length) return 0;
 
     // ...
 }
```

## 3. 内核集成 (`kernel.c`)

我们在 `kmain` 中初始化了文件系统，并读取了那个著名的 `hello.txt`。

```diff
     kfree(ptrB);
     terminal_writestring("Free A&B OK\n\n");
 
+    /* 文件系统测试 */
+    terminal_writestring("Initializing InitRD...\n");
+    fs_root = initrd_init();
+    
+    if (fs_root) {
+        terminal_writestring("Listing files in /:\n");
+        fs_node_t* node = vfs_finddir(fs_root, "hello.txt");
+        if (node) {
+            terminal_writestring("Found: hello.txt\n");
+            /* 读取并打印内容 */
+            uint32_t sz = vfs_read(node, 0, 32, buf);
+            // ...
+            terminal_writestring((char*)buf);
+        }
+    }
 
     /* 多任务测试 */
     process_init();
```

## 4. 构建系统 (`build.sh`)

别忘了把新写的驱动编译进去。

```diff
 x86_64-elf-gcc -m32 -ffreestanding -nostdlib -c process.c -o process.o
+x86_64-elf-gcc -m32 -ffreestanding -nostdlib -c initrd.c -o initrd.o
 
 # 链接所有目标文件
-x86_64-elf-ld -r -m elf_i386 -o core.o kernel.o interrupts.o pmm.o vmm.o heap.o process.o
+x86_64-elf-ld -r -m elf_i386 -o core.o kernel.o interrupts.o pmm.o vmm.o heap.o process.o initrd.o
```

## 5. 成果
系统成功输出了 `Content: Hello VFS World!`，这标志着我们的 OS 正式拥有了**文件系统 (Phase 4)** 的核心能力。
