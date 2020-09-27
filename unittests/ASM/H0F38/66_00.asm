%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x4848484848484848", "0x4848484848484848"],
    "XMM1": ["0x0", "0x0"],
    "XMM2": ["0x0", "0x0"],
    "XMM3": ["0x4847464544434241", "0x5857565554535251"]
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

mov rax, 0x0
mov [rdx + 8 * 2], rax
mov [rdx + 8 * 3], rax

mov rax, -1
mov [rdx + 8 * 4], rax
mov [rdx + 8 * 5], rax

mov rax, 0x8080808080808080
mov [rdx + 8 * 6], rax
mov [rdx + 8 * 7], rax

mov rax, 0x0001020304050607
mov [rdx + 8 * 8], rax
mov rax, 0x08090A0B0C0D0E0F
mov [rdx + 8 * 9], rax

movaps xmm0, [rdx + 8 * 0]
movaps xmm1, [rdx + 8 * 0]
movaps xmm2, [rdx + 8 * 0]
movaps xmm3, [rdx + 8 * 0]

pshufb xmm0, [rdx + 8 * 2]
pshufb xmm1, [rdx + 8 * 4]
pshufb xmm2, [rdx + 8 * 6]
pshufb xmm3, [rdx + 8 * 8]

hlt
