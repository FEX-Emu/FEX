%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x00000000000000FF",
    "MM1": "0x00000000000000FF",
    "MM2": "0x6162636465666778"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x7172737475767778
mov [rdx + 8 * 0], rax
mov rax, 0x4142434445464748
mov [rdx + 8 * 1], rax

mov rax, 0x6162636465666778
mov [rdx + 8 * 2], rax
mov rax, 0x5152535455565748
mov [rdx + 8 * 3], rax

movq mm0, [rdx]
pcmpeqb mm0, [rdx + 8 * 2]

movq mm1, [rdx]
movq mm2, [rdx + 8 * 2]
pcmpeqb mm1, mm2

hlt
