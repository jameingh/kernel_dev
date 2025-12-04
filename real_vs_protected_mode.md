# 16位实模式 vs 32位保护模式：从发明家的视角逐步推导

> 问题：为什么早期 x86 用“16位实模式”启动，而现代操作系统都要切换到“32位保护模式”？从硬件发明与软件需求的演进来解释，并结合本项目的具体实现路径。

## 背景动机
- 1978 年的 8086 只有 16 位寄存器与 20 位地址线，目标是“尽可能简单地让程序跑起来”，因此采用“分段 + 实模式地址计算”的设计。
- 随着程序复杂化，多任务、内存保护、更多地址空间成为刚需；80386 引入“32 位保护模式 + 分页 + 特权环”满足现代 OS 要求。

```text
时间线
8086 (16位，实模式) ──────────► 80286 (保护模式雏形) ──────────► 80386 (32位，保护模式+分页)
```

## 16位实模式：快速启动的最小系统
- 地址计算：`物理地址 = 段寄存器 << 4 + 偏移`，可寻址约 1MB（20 位地址线）。
- 生态：BIOS 中断提供 I/O（例如 `int 0x10` 打印、`int 0x13` 磁盘），启动极简。
- 目标：让 512 字节引导扇区在最简单的环境就能把“更复杂的系统”加载到内存。

```text
实模式内存示意（约 1MB）
----------------------+ 0xFFFFF
| BIOS/设备映射区      |
|                      |
----------------------+ 0xA0000
| 扩展内存/显存        | ← VGA 文本模式 0xB8000
----------------------+ 0x90000
| 引导加载器栈/数据    |
----------------------+ 0x7C00 ← BIOS 将引导扇区装载到这里并执行
| 引导扇区(512B)       |
----------------------+ 0x00000
```

- 项目中的使用：
  - 在实模式下打印与读盘：`/Users/akm/CLionProjects/kernel_dev/boot.asm:160–172`（`int 0x10`），`boot.asm:94–107,109–131`（`int 0x13` LBA/CHS 读磁盘）。
  - 将内核装入内存 `0x10000`：`/Users/akm/CLionProjects/kernel_dev/boot.asm:125–128,221–229`。

## 16位的局限：为什么不直接在实模式做 OS？
- 地址空间太小：约 1MB 可用，现代程序远超此限。
- 没有内存保护：任意程序可写任意地址，操作系统无法隔离错误。
- 多任务困难：缺少硬件支持（特权环、任务切换）、异常处理不完善。
- 依赖 BIOS：接口不统一且性能有限，无法满足现代驱动与调度需求。

## 32位保护模式：为现代 OS 的硬件基石
- 线性地址空间：寄存器 32 位，寻址更广；可结合分页实现虚拟内存。
- 内存保护：通过 GDT/段描述符和分页控制访问，防止越权。
- 特权环：Ring0 内核、Ring3 用户；指令与资源按等级隔离。
- 中断/异常：IDT 定义门，精确分发到 ISR/IRQ，结合任务隔离与恢复。

```text
保护模式结构（核心组件）
- GDT: 段描述符（代码段、数据段、TSS …）
- IDT: 中断描述符表（异常0–31，硬件中断32–47 …）
- Paging: 页目录/页表（后续启用），提供虚拟内存与隔离
- Privilege: Ring0/3（内核/用户）与 CPL/DPL 检查
```

## 从实模式到保护模式：最小可行切换
- 切换步骤：
  1) 关闭中断 `cli`，加载 GDT `lgdt`（告诉 CPU 段的布局）
  2) 置位 `CR0.PE=1` 打开保护模式
  3) 用“远跳转”刷新 `CS` 到 32 位代码段，随后设置各段寄存器到数据段
- 项目中的实现：
  - 切换核心序列：`/Users/akm/CLionProjects/kernel_dev/boot.asm:39–48,59–65`
  - GDT 描述符与选择子：`/Users/akm/CLionProjects/kernel_dev/boot.asm:175–207`
  - 进入 32 位后设置栈并跳到 C：`/Users/akm/CLionProjects/kernel_dev/boot.asm:70–85`

```text
切换示意
实模式              保护模式
cli                 cli
lgdt [gdt]          lgdt [gdt]
mov cr0, cr0|1      CR0.PE = 1（打开保护模式）
jmp 0x08:init_pm    远跳转刷新 CS 到代码段
init_pm: 设置 DS/ES/FS/GS/SS = 0x10（数据段）
        设置栈 ESP/EBP
        call 0x10000（跳到内核）
```

## 进入保护模式后的“神经系统”
- 建立 IDT 并注册 ISR/IRQ：
  - `IDT` 结构与加载：`idt.h` 结构定义，`idt.c:idt_init` 加载；`isr.asm:idt_flush` 完成 `lidt`
  - ISR/IRQ 汇编入口与通用桩：`isr.asm:ISR_NOERRCODE`、`isr.asm:ISR_ERRCODE`、`isr.asm:IRQ`、`isr.asm:isr_common_stub`、`isr.asm:irq_common_stub`
  - C 层处理异常与硬件中断：`interrupts.c:isr_handler`、`interrupts.c:irq_handler`
- 重映射 PIC：将 `IRQ0–15` 映射到 `IDT 32–47`，避免与异常 `0–31` 冲突：`/Users/akm/CLionProjects/kernel_dev/interrupts.c:181–203`

```text
中断分发路径（保护模式）
设备/异常 → 查 IDT[n] → 门项(0x8E) → isr/irq 汇编入口 → 保存现场
                                          ↓
                                   C: isr_handler/irq_handler
                                          ↓
                             （IRQ）发送 EOI 到主/从 PIC
                                          ↓
                                        iret 返回
```

## 设计哲学：为什么“这样搞”是对的
- 分层渐进：用最小的 16 位环境完成“加载工作”，然后切换到更强的 32 位环境做“操作系统工作”。
- 硬件与软件协同：保护模式赋予 OS 控制权与隔离机制；分页与特权环是现代应用稳定性的基础。
- 向前兼容：BIOS/实模式保留启动兼容性；进入保护模式后，OS 接管一切并可逐步摆脱 BIOS。

## 本项目的“发明家路径”总结
- 第1–2周：在实模式用 BIOS 完成“看得见的输出”和“把内核读进来”。
- 第3周：切换到保护模式，建立自己的段与栈，跳入 C。
- 第4–5周：构建 IDT + ISR/IRQ，重映射 PIC，验证中断链路。
- 后续：分页、用户态、系统调用、调度器、文件系统……逐步进化成现代 OS。

---

### 关键源码索引
- 实模式打印与读盘：`/Users/akm/CLionProjects/kernel_dev/boot.asm:94–107,109–131,160–172`
- 切换到保护模式：`/Users/akm/CLionProjects/kernel_dev/boot.asm:39–48,59–65`
- 32 位段与栈设置：`/Users/akm/CLionProjects/kernel_dev/boot.asm:70–85`, `boot.asm:175–207`
- IDT 建立与加载：`idt.c:idt_init`、`isr.asm:idt_flush`
- ISR/IRQ 注册与处理：`interrupts.c:isr_init`、`interrupts.c:irq_init`、`interrupts.c:isr_handler`、`interrupts.c:irq_handler`
