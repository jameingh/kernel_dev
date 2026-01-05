# MCP 级 Linux 内核

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

### 第三阶段：内存管理 (已完成)
- **第6周：物理内存管理 (PMM)**
  - **目标**: 管理物理 RAM。
  - **详情**: 物理页面的位图或栈分配器。
- **第7周：虚拟内存管理 (VMM)**
  - **目标**: 启用分页机制。
  - **详情**: 页目录，页表，高半核映射。
- **第8周：堆管理**
  - **目标**: 动态内核内存分配。
  - **详情**: 实现 `kmalloc` 和 `kfree`。

### 第四阶段：进程管理 (已完成)
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
  - [x] [week1.md](/doc/week1.md)
  - [x] [week2.md](/doc/week2.md)
  - [x] [week3.md](/doc/week3.md)
  - [x] [vga_terminal_summary.md](/doc/vga_terminal_summary.md)
  - [x] [troubleshooting_disk_error.md](/doc/troubleshooting_disk_error.md)

- 第二阶段（神经系统）
  - [x] [real_vs_protected_mode.md](/doc/real_vs_protected_mode.md)
  - [x] [week4.md](/doc/week4.md)
  - [x] [week4_idt_call_chain.md](/doc/week4_idt_call_chain.md)
  - [x] [idt的由来.md](/doc/idt的由来.md)
  - [x] [PIC优先级排序和选择过程.md](/doc/PIC优先级排序和选择过程.md)
  - [x] [why_two_gdt.md](/doc/why_two_gdt.md)
  - [x] [week5.md](/doc/week5.md)

- 第三阶段（内存管理）
  - [x] [week6_pmm_theory.md](/doc/week6_pmm_theory.md)
  - [x] [week7.md](/doc/week7.md)
  - [x] [week7_advanced_mem_map.md](/doc/week7_advanced_mem_map.md)
  - [x] [week8.md](/doc/week8.md)

- 第四阶段（进程管理）
  - [x] [week9.md](/doc/week9.md)

- 第五阶段（存储与用户空间）
  - [x] [week10.md](/doc/week10.md)
  - [x] [week10_implementation.md](/doc/week10_implementation.md)
  - [x] [week10_appendix_scrolling.md](/doc/week10_appendix_scrolling.md)
  - [x] [week11.md](/doc/week11.md)
  - [x] [week11_implementation.md](/doc/week11_implementation.md)
  - [x] [week12.md](/doc/week12.md)
  - [x] [week12_implementation.md](/doc/week12_implementation.md)


## 项目提示词

- 你是一个操作系统内核专家
- 请用中文回答
- 不要直接给出代码，要文档先行
- 涉及到流程图、图示的地方用markdown+mermaid彩图绘制
- mermaid中的subgraph 的标题和节点标签中的特殊字符用双引号括起来，防止解析器报错
- mermaid中的节点标签为包含特殊字符（如 #、/、:、(、)、）和中文的节点标签添加双引号
- 涉及到多个文件改动的小结，给出核心内容的markdown+diff彩色对比示例
- 专业名词请给出中文解释
- 抽象的地方添加详细的图文解释
- 代码请给出详细的中文注释
