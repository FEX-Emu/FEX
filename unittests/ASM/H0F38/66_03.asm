%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x7FFF7FFF7FFF7FFF", "0x800080007FFF7FFF"],
    "XMM1": ["0x800080007FFF7FFF", "0x7FFF7FFF7FFF7FFF"],
    "XMM2": ["0x71836D874331472D", "0x800080007FFF7FFF"],
    "XMM3": ["0x800080007FFF7FFF", "0x71836D874331472D"],
    "XMM4": ["0x7FFF7FFF7FFF7FFF", "0x71836D874331472D"],
    "XMM5": ["0x800080007FFF7FFF", "0x800080007FFF7FFF"],
    "XMM6": ["0x71836D874331472D", "0x800080007FFF7FFF"]
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

movaps xmm0, [rdx + 8 * 0]
movaps xmm1, [rdx + 8 * 2]
movaps xmm2, [rdx + 8 * 4]
movaps xmm3, [rdx + 8 * 6]
movaps xmm4, [rdx + 8 * 0]
movaps xmm5, [rdx + 8 * 2]
movaps xmm6, [rdx + 8 * 4]

phaddsw xmm0, [rdx + 8 * 2]
phaddsw xmm1, [rdx + 8 * 0]

phaddsw xmm2, [rdx + 8 * 2]
phaddsw xmm3, [rdx + 8 * 4]

phaddsw xmm4, [rdx + 8 * 4]
phaddsw xmm5, [rdx + 8 * 6]

phaddsw xmm6, [rdx + 8 * 6]

hlt
