[global gdt_flush]    ; 让C代码可以调用这个函数

gdt_flush:
    mov eax, [esp+4]   ; 获取gdt_ptr的地址
    lgdt [eax]         ; 加载新的GDT
    
    mov ax, 0x10       ; 数据段选择子
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    jmp 0x08:.flush   ; 长跳转刷新CS
.flush:
    mov ax, 0x28      ; TSS 选择子 是 0x28 (5*8)
    ltr ax            ; 加载任务寄存器
    ret
