%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x2020202020202020",
    "MM1": "0x2020202020202020"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x6162636465666768
mov [rdx + 8 * 0], rax
mov rax, 0x7172737475767778
mov [rdx + 8 * 1], rax

mov rax, 0x4142434445464748
mov [rdx + 8 * 2], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 3], rax

movq mm0, [rdx]
psubb mm0, [rdx + 8 * 2]

movq mm1, [rdx]
movq mm2, [rdx + 8 * 2]
psubb mm1, mm2

hlt
