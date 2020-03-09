%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464749",
    "RBX": "0x5152535455565759",
    "RCX": "0x6162636465666769",
    "RDX": "0x7172737475767779",
    "R15": "0x49",
    "R14": "0x5759",
    "R13": "0x65666769",
    "R12": "0x7172737475767779"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov r10, 0xe0000000

mov rax, 0x4142434445464748
mov [r10 + 8 * 0], rax
mov rax, 0x5152535455565758
mov [r10 + 8 * 1], rax
mov rax, 0x6162636465666768
mov [r10 + 8 * 2], rax
mov rax, 0x7172737475767778
mov [r10 + 8 * 3], rax

mov rax, 0x01
xadd  byte [r10 + 8 * 0], al
mov rax, 0x01
xadd  word [r10 + 8 * 1], ax
mov rax, 0x01
xadd dword [r10 + 8 * 2], eax
mov rax, 0x01
xadd qword [r10 + 8 * 3], rax

mov rax, [r10 + 8 * 0]
mov rbx, [r10 + 8 * 1]
mov rcx, [r10 + 8 * 2]
mov rdx, [r10 + 8 * 3]

mov r15, 0x00
xadd  byte [r10 + 8 * 0], r15b
mov r14, 0x00
xadd word [r10 + 8 * 1], r14w
mov r13, 0x00
xadd dword [r10 + 8 * 2], r13d
mov r12, 0x00
xadd qword [r10 + 8 * 3], r12

hlt
