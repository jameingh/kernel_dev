    ; 告诉汇编器，我们的代码会被加载到内存的 0x7C00 位置
    [org 0x7C00]

    ; 设置栈指针，以便我们可以使用 call 和 push 指令
    mov ax, 0       ; 我们不能直接 mov ss, 0，所以通过 ax 中转
    mov ss, ax      ; 栈段寄存器 ss = 0
    mov sp, 0x7C00  ; 栈指针 sp = 0x7C00，栈会从这个地址向下增长

    ; 调用打印字符串的函数
    mov si, HELLO_MSG ; 将要打印的字符串的地址放入 si 寄存器
    call print_string  ; 调用函数

    ; 无限循环，防止CPU继续执行内存后面的垃圾数据
    jmp $

    ; 定义一个打印字符串的函数
    ; 参数: si 寄存器存储着字符串的地址
    print_string:
        mov bh, 0       ; 页号（对于文本模式，总是0）
        mov ah, 0x0E    ; BIOS中断 10h 的功能：TTY模式输出
    .loop:
        mov al, [si]    ; 从 si 指向的内存地址取出一个字符到 al
        cmp al, 0       ; 比较这个字符是不是0（字符串结束符）
        je .done        ; 如果是0，就跳转到 .done 结束
        int 0x10        ; 调用BIOS中断 10h 来打印 al 中的字符
        inc si          ; si 寄存器加1，指向下一个字符
        jmp .loop       ; 跳回 .loop 继续打印
    .done:
        ret             ; 函数返回

    ; 定义我们要打印的数据
    HELLO_MSG db 'Hello, Kernel!', 0 ; 0 是字符串结束符

    ; 填充0，直到第510字节
    times 510-($-$$) db 0

    ; 最后两个字节，必须是 0x55 和 0xAA，这是BIOS识别的“魔法数字”
    dw 0xAA55
    