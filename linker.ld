ENTRY(kmain) /* 指定程序的入口点是kmain函数 */

SECTIONS
{
    . = 0x10000; /* 设置代码段的起始地址为0x10000 */

    .text :
    {
        *(.text) /* 所有输入文件的.text段都放这里 */
    }

    .data :
    {
        *(.data) /* 所有输入文件的.data段都放这里 */
    }

    .bss :
    {
        *(.bss) /* 所有输入文件的.bss段都放这里 */
    }
}
