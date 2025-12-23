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
- 内核主函数初始化顺序：`GDT` → `IDT` → `ISR/IRQ` → `PIT` → 使能中断。
  - 在 `kernel.c:kmain` 中依次调用 `idt_init`、`isr_init`、`irq_init`、`pit_init(100)`。
  - 在 `kernel.c:kmain` 尾部执行 `sti` 使能中断。

## IDT 与门项
- 结构体定义：`idt.h:7–13` 描述 `base_low/sel/flags/base_high`。
- 初始化与加载：
  - 在 `idt.c:idt_init` 设置 `idtp`，清空 256 项后通过 `isr.asm:idt_flush` 加载。
- 门项安装：
  - 异常 `0–31`：在 `interrupts.c:isr_init` 注册 `isr0..isr31`。
  - IRQ `32–47`：在 `interrupts.c:irq_init` 注册 `irq0..irq15`。
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
- 通过 `interrupts.c:pic_remap` 将主/从 PIC 偏移设置为 `0x20`/`0x28`。
- 关键端口与步骤详见 `interrupts.c:pic_remap`。
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
- ISR/IRQ 模板：见 `isr.asm:ISR_NOERRCODE`、`isr.asm:ISR_ERRCODE`、`isr.asm:IRQ`。
- 通用桩保存现场并切换到内核数据段：`isr.asm:isr_common_stub`、`isr.asm:irq_common_stub`。
- 通过 `push esp` 向 C 层传递 `struct registers*`，返回路径 `iret` 同样在上述通用桩中实现。

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
- 异常处理：`interrupts.c:isr_handler` 顶行显示 `EXC XX` 并停机。
- IRQ 处理：`interrupts.c:irq_handler`。
  - 按从→主顺序发送 `EOI`：当 `int_no >= 40` 先 `0xA0` 后 `0x20`。
  - 时钟 `IRQ0`：累加 `ticks` 并周期刷新顶行状态栏文本。
  - 键盘 `IRQ1`：读取 `0x60` 扫描码，处理 `Shift/Caps/Backspace/Enter` 并回显；其余 IRQ 打印向量号。

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
- 启动后提示测试键盘中断：详见 `kernel.c:kmain`。
- 顶行右侧状态栏文本周期刷新：`Hz/Ticks/Keys/Caps/Shift`。
- 键盘字母按 `Shift/Caps` 大小写回显，`Backspace/Enter` 正常工作。

---

### 附：API 索引（用于查阅源代码）
- `kmain` 启动流程：`kernel.c:kmain`
- `idt_set_gate`：`idt.c:idt_set_gate`
- `idt_init` 与 `lidt`：`idt.c:idt_init`、`isr.asm:idt_flush`
- `isr_init`（异常注册）：`interrupts.c:isr_init`
- `pic_remap`：`interrupts.c:pic_remap`
- `irq_init`（硬件中断注册）：`interrupts.c:irq_init`
- `isr_handler`：`interrupts.c:isr_handler`
- `irq_handler`：`interrupts.c:irq_handler`
- `ISR/IRQ 汇编入口与通用桩`：`isr.asm:ISR_NOERRCODE`、`isr.asm:ISR_ERRCODE`、`isr.asm:IRQ`、`isr.asm:isr_common_stub`、`isr.asm:irq_common_stub`
