%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x0202020202020202",
    "MM1": "0x0202020202020202"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

movq mm0, [rdx + 8 * 0]
movq mm1, [rdx + 8 * 1]

phsubw mm0, [rdx + 8 * 1]
phsubw mm1, [rdx + 8 * 0]

hlt
