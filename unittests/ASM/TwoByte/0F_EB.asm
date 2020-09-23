%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x1111111111111111",
    "MM1": "0x1111111111111111",
    "MM2": "0x0101010101010101"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x1010101010101010
mov [rdx + 8 * 0], rax
mov rax, 0x2020202020202020
mov [rdx + 8 * 1], rax

mov rax, 0x0101010101010101
mov [rdx + 8 * 2], rax
mov rax, 0x0202020202020202
mov [rdx + 8 * 3], rax

movq mm0, [rdx]
por mm0, [rdx + 8 * 2]

movq mm1, [rdx]
movq mm2, [rdx + 8 * 2]
por mm1, mm2

hlt
