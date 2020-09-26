%ifdef CONFIG
{
  "RegData": {
    "MM0": "0xA6A8AAAC86888A8C",
    "MM1": "0x86888A8CA6A8AAAC"
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

phaddd mm0, [rdx + 8 * 1]
phaddd mm1, [rdx + 8 * 0]

hlt
