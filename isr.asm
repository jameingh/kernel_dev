[global isr0]
[global isr1]
[global isr2]
[global isr3]
[global isr4]
[global isr5]
[global isr6]
[global isr7]
[global isr8]
[global isr9]
[global isr10]
[global isr11]
[global isr12]
[global isr13]
[global isr14]
[global isr15]
[global isr16]
[global isr17]
[global isr18]
[global isr19]
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
[global isr128]


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

; 32位中断处理程序模板
; 说明：ISR_NOERRCODE 用于无错误码的异常；ISR_ERRCODE 用于带错误码的异常。
; 进入时压入“错误码占位”和“中断号”，供 C 层 isr_handler 读取。
%macro ISR_NOERRCODE 1
isr%1:
    push 0          ; 压入错误码占位符
    push %1         ; 压入中断号
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
isr%1:
    push %1         ; 压入中断号
    jmp isr_common_stub
%endmacro

; IRQ处理程序模板
%macro IRQ 2
;global irq%1
irq%1:
    push 0          ; 压入错误码占位符
    push %2         ; 压入中断号
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

; 中断通用处理程序（汇编桩）：保存现场、切换到内核数据段、将 ESP 作为 struct registers* 传给 C 层。
; 返回前恢复现场并 iret。
extern isr_handler
extern irq_handler

isr_common_stub:
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
    call isr_handler ; 调用C处理函数 (现在返回 struct registers*)
    mov esp, eax    ; <--- 允许在系统调用路径下切换上下文
    
    pop gs          ; 恢复段寄存器
    pop fs
    pop es
    pop ds
    popa            ; 恢复通用寄存器
    add esp, 8      ; 清理错误码和中断号
    iret            ; 中断返回

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
