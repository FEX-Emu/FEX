%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x0202020202020202", "0x0000000000000000"],
    "XMM1": ["0x0000000000000000", "0x0202020202020202"],
    "XMM2": ["0xFF01FF0100FF00FF", "0x0000000000000000"],
    "XMM3": ["0x0000000000000000", "0xFF01FF0100FF00FF"],
    "XMM4": ["0x0202020202020202", "0xFF01FF0100FF00FF"],
    "XMM5": ["0x0000000000000000", "0x0000000000000000"],
    "XMM6": ["0xFF01FF0100FF00FF", "0x0000000000000000"],
    "XMM7": ["0x800080007FFF7FFF", "0x800080007FFF7FFF"]
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

mov rax, 0x7F7F7F7F7F7F7F7F
mov [rdx + 8 * 2], rax
mov rax, 0x8080808080808080
mov [rdx + 8 * 3], rax

mov rax, 0x2119221823172416
mov [rdx + 8 * 4], rax
mov rax, 0x3941384237433644
mov [rdx + 8 * 5], rax

mov rax, 0x7F7F7F7F7F7F7F7F
mov [rdx + 8 * 6], rax
mov rax, 0x8080808080808080
mov [rdx + 8 * 7], rax

mov rax, 0x00007FFF00007FFF
mov [rdx + 8 * 8], rax
mov rax, 0x7FFFFFFF7FFFFFFF
mov [rdx + 8 * 9], rax

movaps xmm0, [rdx + 8 * 0]
movaps xmm1, [rdx + 8 * 2]
movaps xmm2, [rdx + 8 * 4]
movaps xmm3, [rdx + 8 * 6]
movaps xmm4, [rdx + 8 * 0]
movaps xmm5, [rdx + 8 * 2]
movaps xmm6, [rdx + 8 * 4]

movaps xmm7, [rdx + 8 * 8]

phsubsw xmm0, [rdx + 8 * 2]
phsubsw xmm1, [rdx + 8 * 0]

phsubsw xmm2, [rdx + 8 * 2]
phsubsw xmm3, [rdx + 8 * 4]

phsubsw xmm4, [rdx + 8 * 4]
phsubsw xmm5, [rdx + 8 * 6]

phsubsw xmm6, [rdx + 8 * 6]

phsubsw xmm7, [rdx + 8 * 8]

hlt
