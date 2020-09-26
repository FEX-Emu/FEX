%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x31A6343B36E09E7A", "0x48134B294E4F5186"]
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445468748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

mov rax, 0x6162636465666768
mov [rdx + 8 * 2], rax
mov rax, 0x7172737475767778
mov [rdx + 8 * 3], rax

movaps xmm0, [rdx + 8 * 0]

pmulhrsw xmm0, [rdx + 8 * 2]

hlt
