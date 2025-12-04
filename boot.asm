[org 0x7C00]
[bits 16]

; =============================================================================
;                           16位实模式启动代码
; =============================================================================

; 保存BIOS传递的启动驱动器号
mov [BOOT_DRIVE], dl

; 设置栈
mov ax, 0
mov ss, ax
mov sp, 0x7C00

; 显示开始信息
mov si, START_MSG
call print_string

; 检查磁盘是否支持LBA扩展
mov ah, 0x41
mov bx, 0x55AA
mov dl, [BOOT_DRIVE]
int 0x13
jc no_lba_support
cmp bx, 0xAA55
jne no_lba_support

mov si, LBA_MSG
call print_string

; 调用函数从磁盘加载内核
call load_kernel_lba

; 如果成功，显示成功信息
mov si, SUCCESS_MSG
call print_string

; 切换到32位保护模式
cli
lgdt [gdt_descriptor]
mov eax, cr0
or eax, 0x1
mov cr0, eax

; 长跳转到32位代码段
jmp CODE_SEG:init_pm

no_lba_support:
    mov si, NO_LBA_MSG
    call print_string
    call load_kernel_chs
    jmp after_load

after_load:
    mov si, SUCCESS_MSG
    call print_string
    
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:init_pm

[bits 32]
; =============================================================================
;                           32位保护模式代码
; =============================================================================
init_pm:
    ; 更新所有段寄存器
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 设置栈（在32位模式下）
    mov ebp, 0x90000
    mov esp, ebp

    ; 跳转到C代码（内核被加载在0x10000）
    call 0x10000

    ; 如果从C代码返回，进入无限循环
    jmp $

; =============================================================================
;                           回到16位模式函数定义
; =============================================================================
[bits 16]

; --- load_kernel_lba: 使用LBA方式加载内核 ---
load_kernel_lba:
    mov si, LOADING_MSG
    call print_string
    
    ; 设置磁盘地址包 (DAP)
    mov si, disk_address_packet
    
    mov ah, 0x42    ; LBA扩展读取功能
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error
    ret

; --- load_kernel_chs: 使用CHS方式加载内核（备用）---
load_kernel_chs:
    mov si, CHS_MSG
    call print_string
    
    ; 重置磁盘驱动器
    mov ah, 0x00
    mov dl, [BOOT_DRIVE]
    int 0x13
    
    ; 使用传统 CHS 方式
    mov ah, 0x02    ; 读磁盘
    mov al, 64      ; 读取 64 个扇区，确保 kernel.bin 完整载入
    mov ch, 0       ; 柱面0
    mov dh, 0       ; 磁头0  
    mov cl, 2       ; 扇区2 (从1开始计数)
    mov dl, [BOOT_DRIVE] ; 驱动器号
    mov bx, 0x1000  ; ES:BX = 0x1000:0x0000
    mov es, bx
    mov bx, 0
    int 0x13
    jc disk_error
    ret

disk_error:
    mov si, ERROR_MSG
    call print_string
    mov al, ah
    call print_hex
    jmp $

; 新增：打印十六进制数字的函数
print_hex:
    pusha
    mov cx, 4
.hex_loop:
    mov bx, ax
    shr bx, 12
    cmp bl, 9
    jg .letter
    add bl, '0'
    jmp .print
.letter:
    add bl, 'A' - 10
.print:
    mov ah, 0x0E
    int 0x10
    shl ax, 4
    loop .hex_loop
    popa
    ret

; --- print_string: 打印字符串函数 ---
print_string:
    mov bh, 0       ; 页号
    mov ah, 0x0E    ; BIOS中断功能：TTY输出
.loop:
    mov al, [si]    ; 取字符
    cmp al, 0       ; 检查是否结束
    je .done        ; 如果是0，结束
    int 0x10        ; 打印字符
    inc si          ; 指向下一个字符
    jmp .loop       ; 继续循环
.done:
    ret             ; 返回

; =============================================================================
;                           GDT（全局描述符表）
; =============================================================================
gdt_start:
gdt_null:
    dd 0x0          ; 空描述符
    dd 0x0

gdt_code:
    dw 0xFFFF       ; 段限长（低16位）
    dw 0x0          ; 基地址（低16位）
    db 0x0          ; 基地址（中间8位）
    db 10011010b    ; 访问字节：代码段，可读，特权级0
    db 11001111b    ; 标志：32位，粒度4K
    db 0x0          ; 基地址（高8位）

gdt_data:
    dw 0xFFFF       ; 段限长（低16位）
    dw 0x0          ; 基地址（低16位）
    db 0x0          ; 基地址（中间8位）
    db 10010010b    ; 访问字节：数据段，可写，特权级0
    db 11001111b    ; 标志：32位，粒度4K
    db 0x0          ; 基地址（高8位）

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; GDT大小
    dd gdt_start               ; GDT基地址

; 定义段选择子
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; =============================================================================
;                           数据区
; =============================================================================
BOOT_DRIVE db 0
START_MSG db 'Starting...', 13, 10, 0
LBA_MSG db 'LBA supported', 13, 10, 0
NO_LBA_MSG db 'LBA not supported, using CHS', 13, 10, 0
CHS_MSG db 'Using CHS mode', 13, 10, 0
LOADING_MSG db 'Loading kernel...', 13, 10, 0
SUCCESS_MSG db 'Kernel loaded!', 13, 10, 0
ERROR_MSG db 'Disk error! Code: 0x', 0

; 磁盘地址包 (DAP) for LBA
disk_address_packet:
    db 0x10        ; 数据包大小 (16字节)
    db 0           ; 保留字节
    dw 64          ; 要读取的扇区数（与上方 AL 对齐）
    dw 0x0000      ; 缓冲区偏移地址 (ES:BX)
    dw 0x1000      ; 缓冲区段地址
    dd 1           ; 起始LBA扇区号 (从扇区1开始，即第二个扇区)
    dd 0           ; 高32位LBA (对于小磁盘为0)

; =============================================================================
;                           填充和引导签名
; =============================================================================
times 510-($-$$) db 0
dw 0xAA55
