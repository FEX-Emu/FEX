%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFFFFFFFFFFFF48",
    "RBX": "0x41424344454647FF"
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
xchg byte [rdx + 8 * 0], al
mov rbx, [rdx + 8 * 0]

hlt
