%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFFFFFFFFFF4748",
    "RBX": "0x414243444546FFFF"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

mov rax, -1
xchg word [rdx + 8 * 0], ax
mov rbx, [rdx + 8 * 0]

hlt
