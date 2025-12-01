# MCP-Level Linux-Like Kernel

A minimal, functional "Linux-like" kernel implementation project in C and Assembly.

## Project Goal
To build a basic monolithic kernel from scratch, covering the essential components of an operating system: bootloading, memory management, interrupts, multitasking, filesystem, and user mode.

## Development Environment
- **Host**: macOS (Intel/Apple Silicon)
- **Tools**: `qemu`, `i386-elf-gcc`, `nasm`

## Roadmap & Plan

### Phase 1: Boot & Basics (Completed)
- **Week 1: Environment Setup**
  - Install QEMU, GCC cross-compiler, NASM.
- **Week 2: The Bootloader**
  - Write a 512-byte boot sector in Assembly (`boot.asm`).
  - Boot into 16-bit Real Mode.
- **Week 3: Kernel Entry**
  - Switch to 32-bit Protected Mode.
  - Link C code with Assembly.
  - Print "Hello World" to VGA buffer.

### Phase 2: The Nervous System (Planned)
- **Week 4: GDT & IDT**
  - **Goal**: Set up Global Descriptor Table (GDT) and Interrupt Descriptor Table (IDT).
  - **Details**: Define memory segments and interrupt handlers.
- **Week 5: Interrupts & Input**
  - **Goal**: Handle hardware interrupts (PIC) and Keyboard.
  - **Details**: Remap PIC, enable IRQs, PS/2 keyboard driver.

### Phase 3: Memory Management (Planned)
- **Week 6: Physical Memory Management (PMM)**
  - **Goal**: Manage physical RAM.
  - **Details**: Bitmap or Stack allocator for physical pages.
- **Week 7: Virtual Memory Management (VMM)**
  - **Goal**: Enable Paging.
  - **Details**: Page Directory, Page Tables, Higher-half kernel mapping.
- **Week 8: Heap Management**
  - **Goal**: Dynamic kernel memory.
  - **Details**: `kmalloc` and `kfree` implementation.

### Phase 4: Process Management (Planned)
- **Week 9: Multitasking**
  - **Goal**: Scheduler and Context Switching.
  - **Details**: Round-robin scheduler, PCB (Process Control Block).

### Phase 5: Storage & User Space (Planned)
- **Week 10: Filesystem**
  - **Goal**: Virtual File System (VFS).
  - **Details**: InitRD or simple FAT driver.
- **Week 11: User Mode & Syscalls**
  - **Goal**: Ring 3 execution.
  - **Details**: `int 0x80` syscall interface, loading user programs.
- **Week 12: Shell & Polish**
  - **Goal**: Interactive Shell.
  - **Details**: Basic command line interface.

## Build & Run
```bash
# Compile and Link
nasm -f bin boot.asm -o boot.bin
x86_64-elf-gcc -m32 -ffreestanding -c kernel.c -o kernel.o
x86_64-elf-ld -m elf_i386 -T linker.ld -o kernel.bin kernel.o

# Create Image
cat boot.bin kernel.bin > os.img

# Run in QEMU
qemu-system-i386 -hda os.img
```
