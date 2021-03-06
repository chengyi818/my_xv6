```
(gdb) b *0x10000c
Breakpoint 2 at 0x10000c
(gdb) c
Continuing.
=> 0x10000c:    mov    %cr4,%eax

Thread 1 hit Breakpoint 2, 0x0010000c in ?? ()
(gdb) i r
eax            0x0      0
ecx            0x0      0
edx            0x1f0    496
ebx            0x10074  65652
esp            0x7bdc   0x7bdc
ebp            0x7bf8   0x7bf8
esi            0x10074  65652
edi            0x0      0
eip            0x10000c 0x10000c
eflags         0x46     [ PF ZF ]
cs             0x8      8
ss             0x10     16
ds             0x10     16
es             0x10     16
fs             0x0      0
gs             0x0      0
(gdb) x/24x $esp
0x7bdc: 0x00007db4      0x00000000      0x00000000      0x00000000
0x7bec: 0x00000000      0x00000000      0x00000000      0x00000000
0x7bfc: 0x00007c4d      0x8ec031fa      0x8ec08ed8      0xa864e4d0
0x7c0c: 0xb0fa7502      0xe464e6d1      0x7502a864      0xe6dfb0fa
0x7c1c: 0x16010f60      0x200f7c78      0xc88366c0      0xc0220f01
0x7c2c: 0x087c31ea      0x10b86600      0x8ed88e00      0x66d08ec0
```

重新格式化栈内存值

```
0x7bdc: 0x00007db4
    call kernel->entry时,压入的函数返回地址.
0x7be0: 0x00000000
    sub $0xc, %esp的结果
0x7be4: 0x00000000
    sub $0xc, %esp的结果
0x7be8: 0x00000000
    sub $0xc, %esp的结果
0x7bec: 0x00000000
    压入的旧ebx寄存器值
0x7bf0: 0x00000000
    压入的旧esi寄存器值
0x7bf4: 0x00000000
    压入的旧edi寄存器值
0x7bf8: 0x00000000
    压入的旧ebp寄存器值
0x7bfc: 0x00007c4d
    call bootmain时,压入的函数返回地址.
```
从`0x7c00`之后的内容已经是bootloader的可执行代码部分,和函数栈无关矣.
