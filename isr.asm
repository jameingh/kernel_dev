; =============================================================================
; isr.asm - 中断服务例程 (ISR) 与 中断请求 (IRQ) 的底层汇编桩
; =============================================================================
; 对于零基础的同学：
; 1. [global name] 意思是把这个名字暴露给外面（比如 C 语言），让外面能找到它。
; 2. 中断就像是 CPU 运行时的“突发事件”。当事件发生，CPU 会强行停下当前工作，
;    并根据编号跳到这里定义的这些 isr0, isr1... 函数里。
; =============================================================================

[global isr0]   ; 除零异常
[global isr1]   ; 调试异常
[global isr2]   ; 非屏蔽中断
[global isr3]   ; 断点异常
[global isr4]   ; 溢出
[global isr5]   ; 越界
[global isr6]   ; 无效指令
[global isr7]   ; 设备不可用
[global isr8]   ; 双重错误 (有错误码)
[global isr9]   ; 协处理器段越界
[global isr10]  ; 无效 TSS (有错误码)
[global isr11]  ; 段不存在 (有错误码)
[global isr12]  ; 栈段错误 (有错误码)
[global isr13]  ; 通用保护错误 (有错误码 - 之前的 hlt 崩溃就在这)
[global isr14]  ; 页错误 (有错误码)
[global isr15]  ; 保留
[global isr16]  ; 浮点错误
[global isr17]  ; 对齐检查
[global isr18]  ; 机器检查
[global isr19]  ; SIMD 浮点异常
[global isr20]
[global isr21]
[global isr22]
[global isr23]
[global isr24]
[global isr25]
[global isr26]
[global isr27]
[global isr28]
[global isr29]
[global isr30]
[global isr31]
[global isr128] ; 系统调用 (0x80)


[global irq0]
[global irq1]
[global irq2]
[global irq3]
[global irq4]
[global irq5]
[global irq6]
[global irq7]
[global irq8]
[global irq9]
[global irq10]
[global irq11]
[global irq12]
[global irq13]
[global irq14]
[global irq15]

[global idt_flush]

; -----------------------------------------------------------------------------
; 宏 (Macro)：可以理解为汇编版本的“函数模板”
; -----------------------------------------------------------------------------

; 模板 A：用于不带错误码的中断
; 参数 %1 是中断号。我们手动压入一个 0 作为错误码占位，保持栈结构一致。
%macro ISR_NOERRCODE 1
isr%1:
    push 0          ; 压入伪错误码 (为了凑整)
    push %1         ; 压入中断号
    jmp isr_common_stub
%endmacro

; 模板 B：用于带错误码的中断 (CPU 已经压入错误码了)
%macro ISR_ERRCODE 1
isr%1:
    push %1         ; 只压入中断号
    jmp isr_common_stub
%endmacro

; 模板 C：用于硬件中断 (IRQ)
%macro IRQ 2
irq%1:
    push 0          ; 硬件不发错误码，补个 0
    push %2         ; 压入它对应的中断向量号 (32-47)
    jmp irq_common_stub
%endmacro

; 定义所有ISR
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31
ISR_NOERRCODE 128


; 定义所有IRQ (IRQ0-7映射到ISR32-39, IRQ8-15映射到ISR40-47)
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

; -----------------------------------------------------------------------------
; 通用处理桩 (Common Stubs)：这是中断处理的“中转站”
; -----------------------------------------------------------------------------
; 【零基础小贴士】：
; extern 命令的意思是“外部引用”。
; 当汇编看到 extern isr_handler 时，它会明白：“这个函数不在我这，在别的文件（通常是 C 文件）里”。
; 汇编编译器会先留个坑位，最后由【链接器】把这个坑位填上 C 函数的真实内存地址。
; 这样，汇编就可以像“跨界”一样调用 C 语言写的逻辑了。
; -----------------------------------------------------------------------------
extern isr_handler
extern irq_handler

; ISR 通用桩：负责把 CPU 所有的“状态”都保存起来，然后跳到 C 语言里
isr_common_stub:
    pusha           ; 【保存通用寄存器】压入 EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    push ds         ; 【保存段寄存器】记录当前数据段
    push es
    push fs
    push gs
    
    mov ax, 0x10    ; 加载内核数据段选择子 (进入内核的地盘)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp        ; 把当前的栈顶地址 (regs 指针) 传给 C 语言函数
    call isr_handler ; 调用 C 层的处理函数 (比如显示异常号或处理系统调用)
    mov esp, eax    ; 【灵魂一行】如果 C 语言决定换个任务跑，我们就把 ESP 换成新任务的栈
    
    pop gs          ; 恢复段寄存器
    pop fs
    pop es
    pop ds
    popa            ; 恢复通用寄存器
    add esp, 8      ; 跳过栈上的“中断号”和“错误码”
    iret            ; 【中断退出】CPU 从栈里弹出 EIP/CS/EFLAGS，任务“活”过来了

irq_common_stub:
    pusha           ; 保存所有通用寄存器
    push ds         ; 保存段寄存器
    push es
    push fs
    push gs
    
    mov ax, 0x10    ; 加载内核数据段
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp        ; 传递 struct registers* 参数
    call irq_handler ; 调用C处理函数 (返回新的 ESP 到 EAX)
    mov esp, eax    ; <--- 关键点：切换栈指针 (Context Switch)
    
    pop gs          ; 恢复段寄存器 (从新栈)
    pop fs
    pop es
    pop ds
    popa            ; 恢复通用寄存器
    add esp, 8      ; 清理错误码和中断号
    iret            ; 中断返回 (弹出新栈的 EIP)

; 加载 IDT：C 层传入 idtp 指针，通过 lidt 生效
idt_flush:
    mov eax, [esp+4]
    lidt [eax]
    ret
