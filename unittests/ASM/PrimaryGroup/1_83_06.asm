%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x414243444546B8A9",
    "RBX": "0x51525354AAA9A8B9",
    "RCX": "0x9E9D9C9B9A999889"
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
mov rax, 0x6162636465666768
mov [rdx + 8 * 2], rax

xor  word [rdx + 8 * 0 + 0], -31
xor dword [rdx + 8 * 1 + 0], -31
xor qword [rdx + 8 * 2 + 0], -31

mov rax, [rdx + 8 * 0]
mov rbx, [rdx + 8 * 1]
mov rcx, [rdx + 8 * 2]

hlt
