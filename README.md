# MCP 级 Linux 类内核

一个基于 C 和汇编语言从零开始构建的最小化、功能性“类 Linux”内核项目。

## 项目目标
从零开始构建一个基本的宏内核，涵盖操作系统的核心组件：引导加载、内存管理、中断处理、多任务处理、文件系统和用户模式。

## 开发环境
- **主机**: macOS (Intel)
- **工具**: `qemu`, `i386-elf-gcc`, `nasm`

## 路线图与计划

### 第一阶段：引导与基础 (已完成)
- **第1周：环境搭建**
  - 安装 QEMU, GCC 交叉编译器, NASM。
- **第2周：引导加载器 (Bootloader)**
  - 用汇编写一个 512 字节的引导扇区 (`boot.asm`)。
  - 启动进入 16 位实模式。
- **第3周：内核入口**
  - 切换到 32 位保护模式。
  - 链接 C 代码与汇编代码。
  - 向 VGA 显存打印“Hello World”。

### 第二阶段：神经系统 (已完成)
- **第4周：GDT 与 IDT**
  - **目标**: 设置全局描述符表 (GDT) 和中断描述符表 (IDT)。
  - **详情**: 定义内存段和中断处理程序。
- **第5周：中断与输入**
  - **目标**: 处理硬件中断 (PIC) 和键盘输入。
  - **详情**: 重映射 PIC，启用 IRQ，PS/2 键盘驱动。

### 第三阶段：内存管理 (计划中)
- **第6周：物理内存管理 (PMM)**
  - **目标**: 管理物理 RAM。
  - **详情**: 物理页面的位图或栈分配器。
- **第7周：虚拟内存管理 (VMM)**
  - **目标**: 启用分页机制。
  - **详情**: 页目录，页表，高半核映射。
- **第8周：堆管理**
  - **目标**: 动态内核内存分配。
  - **详情**: 实现 `kmalloc` 和 `kfree`。

### 第四阶段：进程管理 (计划中)
- **第9周：多任务处理**
  - **目标**: 调度器和上下文切换。
  - **详情**: 轮转调度算法 (Round-robin)，PCB (进程控制块)。

### 第五阶段：存储与用户空间 (计划中)
- **第10周：文件系统**
  - **目标**: 虚拟文件系统 (VFS)。
  - **详情**: InitRD 或简单的 FAT 驱动。
- **第11周：用户模式与系统调用**
  - **目标**: Ring 3 执行权限。
  - **详情**: `int 0x80` 系统调用接口，加载用户程序。
- **第12周：Shell 与完善**
  - **目标**: 交互式 Shell。
  - **详情**: 基本的命令行接口。

## 构建与运行
```bash
./build.sh
```

## 学习文档
- 说明：文档中的代码引用统一采用“文件:函数名”的跳转提示（如 `interrupts.c:irq_handler`），避免行号随代码改动而失效。
- 第一阶段（引导与基础）
  - [week1.md](./week1.md)
  - [week2.md](./week2.md)
  - [week3.md](./week3.md)
  - [vga_terminal_summary.md](./vga_terminal_summary.md)
  - [troubleshooting_disk_error.md](./troubleshooting_disk_error.md)

- 第二阶段（神经系统）
  - [real_vs_protected_mode.md](./real_vs_protected_mode.md)
  - [week4.md](./week4.md)
  - [week4_idt_call_chain.md](./week4_idt_call_chain.md)
  - [idt的由来.md](./idt的由来.md)
  - [PIC优先级排序和选择过程.md](./PIC优先级排序和选择过程.md)
  - [why_two_gdt.md](./why_two_gdt.md)
  - [week5.md](./week5.md)

- 第三阶段（内存管理）
  - [week6_pmm_theory.md](./week6_pmm_theory.md)
