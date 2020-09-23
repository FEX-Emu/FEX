%ifdef CONFIG
{
  "RegData": {
    "MM0": "0xFFFFFFFFFFFFFFFF",
    "MM1": "0xFFFFFFFFFFFFFFFF",
    "MM2": "0x6162636465666768"
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

mov rax, 0x6162636465666768
mov [rdx + 8 * 2], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 3], rax

movq mm0, [rdx]
pcmpgtd mm0, [rdx + 8 * 2]

movq mm1, [rdx]
movq mm2, [rdx + 8 * 2]
pcmpgtd mm1, mm2

hlt
