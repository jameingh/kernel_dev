# 断点中断（int 3）历史说明与当前实现差异
- 早期版本曾在内核中触发 `int $3` 以验证 IDT/ISR 链路。
- 当前版本已移除该触发，改为通过 `PIT` 心跳与键盘输入验证中断系统。

**执行路径**
**执行路径（历史版本参考）**
- CPU 根据 IDT 查找向量 3 的门项，跳到 `isr.asm:isr3` 汇编入口。
- 汇编通用桩保存现场并调用 C 层处理函数：`isr.asm:isr_common_stub`。
- C 层在 `interrupts.c:isr_handler` 中显示顶行 `EXC XX` 并停机。

**相关初始化**
- 向量 3 的门项在 IDT 初始化时被注册：`interrupts.c:isr_init`
- 启用中断用于验证中断系统：`kernel.c:kmain` 中的 `sti`

**补充**
- `int $3` 是 AT&T 语法，`$3` 表示立即数 3；这是经典的“断点”中断，常用于调试或验证中断机制。

- 不需要在 VSCode 里手动设置断点；`int $3` 本身就是触发“断点中断”（Trap 3）的软件指令，相当于主动命中一个断点。
- 你的内核用它来验证 IDT/ISR 链路是否工作，并在处理后用 `hlt` 停住，便于观察。

**两行代码的作用**
- `kernel.c:34` 的 `int $3`：向量 3 的软件中断，CPU 查 `IDT[3]` 跳到 `isr3`（`/Users/akm/CLionProjects/kernel_dev/isr.asm:80–81`），随后进入通用桩并调到 C 处理函数（`/Users/akm/CLionProjects/kernel_dev/isr.asm:132–147`）。
- `interrupts.c:41–43` 的 `while(1) hlt;`：在 `int 3` 分支里打印提示后让 CPU进入“停机”状态，直到下一个中断再唤醒，又立刻继续 `hlt`，形成稳定停机。这是为了调试“返回路径是否正确”（不走 `iret`，屏幕更稳定）。

**为什么不需要 VSCode 断点**
- 这是裸机内核，运行在 QEMU/真实机上；`int $3` 直接走硬件中断流程，比 IDE 断点更贴近真实机制。
- 你当前的 `isr_handler` 对 `int_no == 3` 有专门分支（`/Users/akm/CLionProjects/kernel_dev/interrupts.c:32–44`），因此能清晰看到“断点中断已触发”的视觉结果。

**如果你想做源级调试**
- 可用 QEMU 的 GDB stub 进行远程调试，然后用 GDB 或 VSCode C/C++ 插件连上 `:1234` 设置源级断点，例如在 `isr3` 或 `kmain`。这属于额外配置，和 `int $3` 的机制独立共存。

**结论**

![idt.png](/imgs/idt.png)
- 截图里的界面是符合当前设计的。触发 `int 3` 后，异常处理会在屏幕左上角覆盖一段文本并进入停机循环，所以画面稳定不再滚动是预期行为。

**预期视觉效果**
- 顶行显示异常号 `EXC XX` 并停机：`interrupts.c:isr_handler`。
- 初始化日志：`kernel.c:kmain`。

**原因说明**
- IDT 跳转：CPU 查找向量后进入 `isrX`：`isr.asm:ISR_NOERRCODE/ISR_ERRCODE`。
- C 层处理：异常显示顶行并停机：`interrupts.c:isr_handler`。

**提示**
- 当前版本通过状态栏与键盘回显验证中断链路：`interrupts.c:draw_status`、`interrupts.c:irq_handler`（IRQ0/IRQ1 分支）。
