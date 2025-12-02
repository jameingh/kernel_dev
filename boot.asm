[org 0x7C00]

mov [BOOT_DRIVE], dl ; BIOS会把启动磁盘号存在dl里，我们把它保存下来

mov ax, 0
mov ss, ax
mov sp, 0x7C00

; 调用我们新写的函数，从磁盘加载内核
call load_kernel

; 跳转到我们加载内核的地址 (0x10000)
jmp 0x10000

; 无限循环，防止出错
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

; 定义一个从磁盘加载内核的函数
; 使用 BIOS int 0x13, ah=0x02 功能
load_kernel:
    mov ah, 0x02    ; BIOS功能号：读磁盘
    mov al, 10       ; 精确读取内核所需的10个扇区
    mov cl, 2        ; 从第2个扇区开始读 (LBA 1)
    mov ch, 0        ; 柱面号
    mov dh, 0        ; 磁头号
    mov dl, [BOOT_DRIVE] ; 驱动器号
    mov bx, 0x1000   ; ES:BX = 0x1000:0x0000，即物理地址 0x10000
    mov es, bx
    mov bx, 0
    int 0x13         ; 调用BIOS中断
    jc disk_error    ; 如果进位标志位被设置，说明出错了
    ret

disk_error:
    ; 如果出错，就在屏幕上打印一个 'X'
    mov bh, 0
    mov ah, 0x0E
    mov al, 'X'
    int 0x10
    jmp $

; 数据区
BOOT_DRIVE db 0

; 填充0和魔法数字
times 510-($-$$) db 0
dw 0xAA55
