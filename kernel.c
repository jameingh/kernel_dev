void kmain() {
    volatile char* vga = (volatile char*)0xB8000;
    vga[0] = 'H';
    vga[1] = 0x0F;
    vga[2] = 'i';
    vga[3] = 0x0F;
    
    while(1) {}
}
