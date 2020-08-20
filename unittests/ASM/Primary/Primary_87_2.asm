%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000045464748",
    "RBX": "0x41424344FFFFFFFF"
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
xchg dword [rdx + 8 * 0], eax
mov rbx, [rdx + 8 * 0]

hlt
