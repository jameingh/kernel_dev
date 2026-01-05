# 内核调度优化设计：从“忙等待”到“主动让位” (Yield)

## 1. 背景与目标
目前内核的 `user_task` 采用 `for` 循环忙等待来消耗时间。这种做法：
- 浪费 CPU 周期。
- 占用整个时间片，降低系统整体吞吐量。
- 增加功耗和发耗。

目标是实现 `sys_yield` 系统调用，允许任务自愿放弃剩余的时间片，将控制权交给调度器。

## 2. 技术设计

### A. 系统调用接口
- **调用号**：`2` (SYSCALL_YIELD)
- **行为**：任务调用后，内核立即保存当前上下文，并切换到下一个就绪任务，而不必等待时钟中断（PIT）的强制切换。

### B. 支撑机制：内核态上下文切换扩展
目前内核只有在 `IRQ`（时钟/键盘中断）路径下支持返回新栈指针（用于进程切换）。
```asm
; 当前 isr_common_stub (syscall 路径)
call isr_handler
; 缺少 mov esp, eax，无法切换任务
```
**改进**：修改 `isr_common_stub`，使其支持 `isr_handler` 返回的新寄存器指针，从而允许系统调用直接触发任务切换。

### C. 调度逻辑
- `schedule()` 函数已经实现了轮转调度（Round Robin）。
- `sys_yield` 只需简单地调用 `schedule(current_regs)` 并将其返回的新寄存器集返回给汇编桩。

## 3. 实现路线图

### 第一阶段：基础设施修改
1. **修改 `isr.asm`**：统一 `isr_common_stub` 与 `irq_common_stub` 的行为，支持 `mov esp, eax`。
2. **修改 `interrupts.c`**：将 `isr_handler` 的返回值类型由 `void` 改为 `struct registers*`。

### 第二阶段：增加系统调用
3. **修改 `syscall.c`**：增加 `eax == 2` 的处理逻辑，调用调度器。
4. **修改 `kernel.c`**：在 `user_task` 中使用 `int $0x80` 发起 Yield。

### 第三阶段：验证
5. **观察 QEMU**：验证 Yield 发生后，CPU 能立即流转到下游任务。

## 4. 未来展望：信号量与睡眠队列
一旦有了 Yield，下一步就是实现 `sys_sleep(ms)`。这需要：
- 引入“阻塞（Blocked）”进程状态。
- 维护一个基于 Tick 的等待链表。
- 调度器在选择下个进程时跳过非就绪态的进程。
