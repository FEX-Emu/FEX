%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x65666768",
    "RBX": "0x6162636465666768",
    "RCX": "0x75767778",
    "RSI": "0x7172737475767778"
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
mov rax, 0x7172737475767778
mov [rdx + 8 * 3], rax

mov rax, 0x0
mov [rdx + 8 * 4], rax
mov [rdx + 8 * 5], rax

movups xmm2, [rdx + 8 * 2]
movups xmm3, [rdx + 8 * 3]

movd dword [rdx + 8 * 4], xmm2
; AMD's Architecture programmer's manual claims this mnemonic is still movd, but compilers only accept movq
movq qword [rdx + 8 * 5], xmm2

mov rax, [rdx + 8 * 4]
mov rbx, [rdx + 8 * 5]

movd ecx, xmm3
movq rsi, xmm3

hlt
