# 第4周：中断处理系统图文讲解

> 目标：从硬件信号到内核 C 处理函数的完整路径，涵盖 IDT 建立、异常（ISR）与硬件中断（IRQ）分发、PIC 重映射与 EOI、以及当前项目的实际行为与验证方法。

## 概览
- 建立 `IDT` 并为异常 `0–31` 与硬件中断 `32–47` 设置门项。
- 通过 `isr.asm` 提供 ISR/IRQ 汇编入口，将寄存器快照打包为 `struct registers*` 传递到 C 层。
- 通过 `PIC` 重映射把 `IRQ0–15` 对齐到 `IDT` 向量 `32–47`，避免与 CPU 异常区间冲突。
- C 层行为：异常统一打印并停机；断点 `int 3` 特殊显示；IRQ 发送 EOI 并打印向量号，临时忽略时钟刷屏。
- 内核在启动流程中初始化 `GDT`、`IDT`、注册 ISR/IRQ，`sti` 使能中断，并触发 `int 3` 进行可视化验证。

```text
┌─────────────────────────────── 硬件到内核路径 ────────────────────────────────┐
│ 设备/CPU 异常  ──>  CPU 查 IDT[n]  ──>  门项(0x8E)  ──>  isr/irq 汇编入口       │
│                                        │           └─> common_stub 保存现场  │
│                                        │                           │         │
│                                        └────────────── push esp ───┘         │
│                                                               │              │
│                                              C: isr_handler/irq_handler      │
│                                                               │              │
│                           IRQ: 发送 EOI 到主/从 PIC           │              │
│                                                               │              │
│                                              恢复现场 iret 返回              │
└──────────────────────────────────────────────────────────────────────────────┘
```

## 启动与初始化
- 内核主函数初始化顺序：`GDT` → `IDT` → `ISR/IRQ` → 使能中断 → 触发断点。
  - `kernel.c:20–24` 依次调用 `idt_init()`、`isr_init()`、`irq_init()`。
  - `kernel.c:29` 使能中断 `sti`；`kernel.c:33–34` 触发 `int 3` 测试。

## IDT 与门项
- 结构体定义：`idt.h:7–13` 描述 `base_low/sel/flags/base_high`。
- 初始化与加载：
  - `idt.c:18–29` 设置 `idtp`，清空 256 项后通过 `lidt` 加载（`isr.asm:183–186`）。
- 门项安装：
  - 异常 `0–31`：`interrupts.c:85–118` 将 `isr0..isr31` 注册到 IDT。
  - IRQ `32–47`：`interrupts.c:187–203` 将 `irq0..irq15` 注册到 IDT。
- 标志 `0x8E`：表示“存在位=1、DPL=0、类型=0xE（32位中断门）”，选择子 `0x08` 指向内核代码段。

```text
IDT[n] 门项布局（32位）
- base_low   : 处理函数地址低 16 位
- sel        : 段选择子（内核代码段 0x08）
- always0    : 固定为 0
- flags      : 0x8E（P=1,DPL=0,Type=1110b）
- base_high  : 处理函数地址高 16 位
```

## PIC 重映射与 IRQ 区间
- 原始 8259 PIC 将 `IRQ0–7` 映射到 `0x08–0x0F`，与 CPU 异常区间 `0x00–0x1F` 冲突。
- 通过重映射将主/从 PIC 偏移设置为 `0x20`/`0x28`：`interrupts.c:181–185`。
- 关键端口与步骤：`interrupts.c:131–179`
  - 读取并保存掩码：`PIC1_DATA(0x21)`、`PIC2_DATA(0xA1)`。
  - 发送 `ICW1/2/3/4` 完成重映射与 8086 模式设置。
  - 恢复原掩码，避免误启用未期望的 IRQ。

```text
IRQ 映射关系（重映射后）
IRQ0  → IDT 32  (0x20)  时钟
IRQ1  → IDT 33  (0x21)  键盘
IRQ2  → IDT 34  (0x22)  级联
IRQ3  → IDT 35  (0x23)  串口2
IRQ4  → IDT 36  (0x24)  串口1
IRQ5  → IDT 37  (0x25)  LPT2/保留
IRQ6  → IDT 38  (0x26)  软盘
IRQ7  → IDT 39  (0x27)  LPT1/保留
IRQ8  → IDT 40  (0x28)  CMOS 实时钟
IRQ9  → IDT 41  (0x29)  重用 IRQ2
IRQ10 → IDT 42  (0x2A)  保留/设备
IRQ11 → IDT 43  (0x2B)  保留/设备
IRQ12 → IDT 44  (0x2C)  PS/2 鼠标
IRQ13 → IDT 45  (0x2D)  FPU
IRQ14 → IDT 46  (0x2E)  主 IDE
IRQ15 → IDT 47  (0x2F)  从 IDE
```

## 汇编入口与通用桩
- ISR/IRQ 模板：`isr.asm:54–74` 将“错误码占位+中断号”压栈后跳到通用桩。
- 通用桩保存现场并切换到内核数据段：`isr.asm:132–145`。
- 通过 `push esp` 向 C 层传递 `struct registers*`：`isr.asm:145–147,170–172`。
- 返回路径：清理栈、恢复寄存器、`iret`：`isr.asm:149–155,174–180`。

```text
栈布局（进入 C 层前后，示意）
┌───────────────┐  ← ESP
│  gs,fs,es,ds  │  pusha+段寄存器保存
│  通用寄存器   │
│  err_code     │  ISR: 可能为占位 0；某些异常真实错误码
│  int_no       │  ISR/IRQ 向量号（例如 0x21）
└───────────────┘
      ↓ push esp 传入 C：`struct registers*`
```

## C 层行为与可视化
- 异常处理：`interrupts.c:11–55`
  - `int 3`（断点，IDT 3）：绿色提示后 `hlt` 死循环，便于定位返回路径问题。
  - 其它异常：红色 `EXCEPTION HALT` 并停机。
- IRQ 处理：`interrupts.c:59–79`
  - 按从→主顺序发送 `EOI`：当 `int_no >= 40` 先 `0xA0` 后 `0x20`。
  - 忽略时钟 `IRQ0` 的打印，防止刷屏：`int_no == 32` 直接返回。
  - 打印收到的 IRQ 向量（十六进制两位）：例如键盘为 `0x21` 显示 `Received IRQ: 21`。

```text
异常 vs. IRQ 处理流程
设备/异常 → IDT → isr/irq → 保存现场
    ├─ 异常：显示/停机（int3 特殊）
    └─ IRQ ：发送 EOI → 可选日志 → 返回
```

## 关键结构
- `struct registers`：`interrupts.h:7–12`，完整保存段寄存器、通用寄存器、向量号、错误码、返回现场。
- `struct idt_entry`：`idt.h:7–13`，描述每个门项的地址与属性。

## 当前完成度与待完善
- 键盘：尚未解析扫描码，只记录向量号；可在 `IRQ1` 读取 `0x60` 扫描码并映射字符。
- 时钟：未做调度或节拍计数；后续可注册 `tick` 计数与任务切换钩子。
- 异常：统一停机；后续可为常见异常（页错误、通用保护等）打印寄存器详情与恢复策略。

## 快速验证
- 启动后提示测试键盘中断：`kernel.c:26–28`。
- 按任意键观察日志：`Received IRQ: 21`（键盘 IRQ1 → IDT 33 → 十六进制 `0x21`）。
- 断点测试：屏幕显示 `INT: 03 Breakpoint (HALT)` 并停机，说明 IDT/ISR 路径正常。

---

### 附：API 索引（用于查阅源代码）
- `kmain` 启动流程：`/Users/akm/CLionProjects/kernel_dev/kernel.c:20–34`
- `idt_set_gate`：`/Users/akm/CLionProjects/kernel_dev/idt.c:8–15`
- `idt_init` 与 `lidt`：`/Users/akm/CLionProjects/kernel_dev/idt.c:18–29`、`/Users/akm/CLionProjects/kernel_dev/isr.asm:183–186`
- `isr_init`（异常注册）：`/Users/akm/CLionProjects/kernel_dev/interrupts.c:84–118`
- `pic_remap`：`/Users/akm/CLionProjects/kernel_dev/interrupts.c:150–179`
- `irq_init`（硬件中断注册）：`/Users/akm/CLionProjects/kernel_dev/interrupts.c:181–203`
- `isr_handler`：`/Users/akm/CLionProjects/kernel_dev/interrupts.c:11–55`
- `irq_handler`：`/Users/akm/CLionProjects/kernel_dev/interrupts.c:59–79`
- `ISR/IRQ 汇编入口与通用桩`：`/Users/akm/CLionProjects/kernel_dev/isr.asm:54–74,132–155,157–180`
