

### 安装汇编编译器

```bash
brew install nasm
```

### 第一步：将boot.asm编译成boot.bin
```bash
nasm -f bin boot.asm -o boot.bin
```

`-f bin`：告诉NASM生成一个纯二进制文件，没有任何额外的格式信息，这正是我们需要的512字节引导扇区。

### 第二步：用QEMU运行！

```bash
qemu-system-i386 -hda boot.bin
```

*   `-hda boot.bin`：告诉QEMU，把 `boot.bin` 这个文件当作第一块硬盘来启动。

*   **成功的标志：** 一个QEMU的窗口会弹出，在黑色的屏幕上，你会看到白色的文字：**`Hello, Kernel!`**

