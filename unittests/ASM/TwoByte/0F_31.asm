%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov r15, 0xe0000000

mov rax, 0x0
mov [r15 + 8 * 0], rax

rdtsc
shl rdx, 32
or rax, rdx
cmp rax, 0
setne [r15 + 8 * 0]
mov rax, [r15 + 8 * 0]

hlt
