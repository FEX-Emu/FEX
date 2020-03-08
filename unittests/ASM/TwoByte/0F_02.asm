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

mov rdx, 0xe0000000

mov rax, 0x0
mov [rdx + 8 * 0], rax

mov rax, 201 ; Time
syscall
cmp rax, 0
setne [rdx + 8 * 0]
mov rax, [rdx + 8 * 0]

hlt
