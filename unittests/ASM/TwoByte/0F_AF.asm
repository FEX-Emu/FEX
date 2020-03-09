%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x5C00",
    "RBX": "0x54D45400",
    "RSI": "0x4ECE4DCD4CCC4C00"
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

mov rax, 0x0
mov [r15 + 8 * 3], rax
mov [r15 + 8 * 4], rax

mov ax, -128
imul ax, word [r15 + 8 * 0 + 0]
mov word [r15 + 8 * 3 + 0], ax

mov eax, -128
imul eax, dword [r15 + 8 * 1 + 0]
mov dword [r15 + 8 * 4 + 0], eax

mov rax, -128
imul rax, qword [r15 + 8 * 2 + 0]
mov rsi, rax

mov rax, [r15 + 8 * 3]
mov rbx, [r15 + 8 * 4]

hlt
