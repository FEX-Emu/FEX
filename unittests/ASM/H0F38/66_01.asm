%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0xA4A6ACAE84868C8E", "0xE4E6ECEEC4C6CCCE"],
    "XMM1": ["0xE4E6ECEEC4C6CCCE", "0xA4A6ACAE84868C8E"]
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

phaddw xmm0, [rdx + 8 * 2]
phaddw xmm1, [rdx + 8 * 0]

hlt
