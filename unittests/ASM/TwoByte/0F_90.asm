%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1",
    "RBX": "0x0"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov r15, 0xe0000000

mov r10, 0x80000000
mov r11, 0x1
mov r12, 0x2

cmp r10d, r12d

mov rax, 0
mov rbx, 0
seto al
setno bl

hlt
