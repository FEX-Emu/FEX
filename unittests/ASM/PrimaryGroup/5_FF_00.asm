%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464749",
    "RBX": "0x5152535455565759",
    "RCX": "0x6162636465666769"
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

inc  word [r15 + 8 * 0 + 0]
inc dword [r15 + 8 * 1 + 0]
inc qword [r15 + 8 * 2 + 0]

mov rax, [r15 + 8 * 0]
mov rbx, [r15 + 8 * 1]
mov rcx, [r15 + 8 * 2]

hlt
