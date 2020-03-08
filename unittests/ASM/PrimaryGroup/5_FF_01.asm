%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464747",
    "RBX": "0x5152535455565757",
    "RCX": "0x6162636465666767"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov r15, 0xe0000000

mov rax, 0x4142434445464748
mov [r15 + 8 * 0], rax
mov rax, 0x5152535455565758
mov [r15 + 8 * 1], rax
mov rax, 0x6162636465666768
mov [r15 + 8 * 2], rax

dec  word [r15 + 8 * 0 + 0]
dec dword [r15 + 8 * 1 + 0]
dec qword [r15 + 8 * 2 + 0]

mov rax, [r15 + 8 * 0]
mov rbx, [r15 + 8 * 1]
mov rcx, [r15 + 8 * 2]

hlt
