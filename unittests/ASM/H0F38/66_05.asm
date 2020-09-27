%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0xF202F20212021202", "0x0202020202020202"],
    "XMM1": ["0x0202020202020202", "0xF202F20212021202"]
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x5142634475468748
mov [rdx + 8 * 0], rax
mov rax, 0x5152435435562758
mov [rdx + 8 * 1], rax
mov rax, 0x6172637465766778
mov [rdx + 8 * 2], rax
mov rax, 0x7162736475667768
mov [rdx + 8 * 3], rax

movaps xmm0, [rdx + 8 * 0]
movaps xmm1, [rdx + 8 * 2]

phsubw xmm0, [rdx + 8 * 2]
phsubw xmm1, [rdx + 8 * 0]

hlt
