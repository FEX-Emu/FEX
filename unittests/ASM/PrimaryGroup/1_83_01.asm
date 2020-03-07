%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x414243444546FFE9",
    "RBX": "0x51525354FFFFFFF9",
    "RCX": "0xFFFFFFFFFFFFFFE9"
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

or  word [rdx + 8 * 0 + 0], -31
or dword [rdx + 8 * 1 + 0], -31
or qword [rdx + 8 * 2 + 0], -31

mov rax, [rdx + 8 * 0]
mov rbx, [rdx + 8 * 1]
mov rcx, [rdx + 8 * 2]

hlt
