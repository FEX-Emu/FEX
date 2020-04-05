%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464748",
    "RBX": "0x4142434445464748",
    "RCX": "0x0000000045464748"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x0
mov [rdx + 8 * 1], rax
mov [rdx + 8 * 2], rax

mov rax, [rdx + 8 * 0]
movnti [rdx + 8 * 1], rax
movnti [rdx + 8 * 2], eax

mov rbx, [rdx + 8 * 1]
mov rcx, [rdx + 8 * 2]

hlt
