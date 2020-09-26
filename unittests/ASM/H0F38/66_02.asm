%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0xA6A8AAAC86888A8C", "0xE6E8EAECC6C8CACC"],
    "XMM1": ["0xE6E8EAECC6C8CACC", "0xA6A8AAAC86888A8C"]
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

movaps xmm0, [rdx + 8 * 0]
movaps xmm1, [rdx + 8 * 2]

phaddd xmm0, [rdx + 8 * 2]
phaddd xmm1, [rdx + 8 * 0]

hlt
