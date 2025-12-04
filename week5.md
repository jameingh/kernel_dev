# 第二阶段总结：中断与输入系统的完善与稳定化

> 目标：完成“神经系统”阶段的功能与可视化，包括 IRQ 分发、键盘输入、PIT 心跳与状态栏，减少视觉闪烁并提升稳定性。

## 完成项
- IRQ 分发与 EOI：主/从 PIC 正确发送 EOI，避免阻塞后续中断。
- 键盘输入（IRQ1）：读取端口 `0x60` 扫描码，解析 set1 make/break；支持 `Enter`、`Backspace`、`Shift`、`Caps Lock`；字母大小写依据 `Shift XOR Caps`。
- 时钟（IRQ0）：初始化 PIT 为 `100 Hz`，维护 tick 计数并驱动状态栏刷新。
- 状态栏：固定第一行右侧显示 `Hz/Ticks/Keys/Caps/Shift`，定宽、定域写入，避免滚屏与闪烁。
- 显示稳定化：禁用 VGA 硬件光标；异常路径直写 VGA 顶行输出两位异常号，便于定位。
- 构建与引导稳定化：增大内核装载扇区与镜像容量，避免“内核未完整加载”导致的重启闪屏。

## 代码索引
- IRQ 分发与 EOI：`/Users/akm/CLionProjects/kernel_dev/interrupts.c:107–112`
- 键盘输入：
  - IRQ1 分支与状态更新：`/Users/akm/CLionProjects/kernel_dev/interrupts.c:121–132`
  - 扫描码到字符映射：`/Users/akm/CLionProjects/kernel_dev/interrupts.c:44–97`
  - Backspace 行内回退与擦除：`/Users/akm/CLionProjects/kernel_dev/terminal.c:66–75`
- PIT 时钟：
  - 初始化与频率记录：`/Users/akm/CLionProjects/kernel_dev/interrupts.c:99–105`
  - Tick 计数与刷新时机：`/Users/akm/CLionProjects/kernel_dev/interrupts.c:113–118`
- 状态栏文本（Hz/Ticks/Keys/Caps/Shift）：`/Users/akm/CLionProjects/kernel_dev/interrupts.c:30–46`
- 异常直写 VGA 顶行：`/Users/akm/CLionProjects/kernel_dev/interrupts.c:16–26`
- PIC 重映射与掩码：`/Users/akm/CLionProjects/kernel_dev/interrupts.c:208–229`
- IRQ 门项注册：`/Users/akm/CLionProjects/kernel_dev/interrupts.c:231–253`
- 禁用硬件光标：`/Users/akm/CLionProjects/kernel_dev/terminal.c:12–20,31–43`
- 内核初始化调用序：`/Users/akm/CLionProjects/kernel_dev/kernel.c:20–25,31–36`
- 引导加载稳定化（加载更多扇区）：`/Users/akm/CLionProjects/kernel_dev/boot.asm:120,224`
- 镜像容量扩大：`/Users/akm/CLionProjects/kernel_dev/build.sh:26`

## 行为与验证
- 顶行右侧状态栏文本按固定间隔刷新：`Hz:100 Ticks:000123 Keys:0010 Caps:N Shift:Y`。
- 键盘：
  - 字母大小写受 `Shift/Caps` 影响；
  - `Enter` 换行；`Backspace` 行内回退并擦除。
- 时钟：每 10 tick 更新一次状态栏，屏幕不滚动、不闪烁。
- 异常：若发生，左上角显示 `EXC XX` 两位十六进制异常号并停机，利于定位。

## 设计要点
- 状态栏采用“定宽字符串 + 固定位置写入”，仅覆盖一个小区域，最大限度减少视觉抖动。
- 仅开放必要 IRQ（主 PIC 掩码：`0xF8`；从 PIC 全屏蔽），降低噪声中断带来的干扰。
- 终端接口在键盘字符回显与行控制中使用，异常路径避免依赖终端，提升早期错误可见性。

## 后续计划（第三阶段预告）
- 物理内存管理（PMM）：内存检测、页帧位图/栈式分配器。
- 虚拟内存管理（VMM）：开启分页、页目录/页表、高半核映射、基本 `kmalloc`。

---

### 附：关键调用链
- `IRQ1`：键盘设备 → `PIC` → `IDT[33]` → `irq1` → `irq_handler` 读取 `0x60` → 回显字符（`terminal_putchar`）
- `IRQ0`：`PIT` → `PIC` → `IDT[32]` → `irq0` → `irq_handler` 累加 `pit_ticks` → 刷新状态栏
