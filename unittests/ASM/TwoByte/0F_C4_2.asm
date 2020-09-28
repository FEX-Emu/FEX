%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x4142434445467778",
    "MM1": "0x4142434477784748",
    "MM2": "0x4142777845464748",
    "MM3": "0x7778434445464748",
    "MM4": "0x4142434445467778",
    "MM5": "0x4142434477784748",
    "MM6": "0x4142777845464748",
    "MM7": "0x7778434445464748"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

mov rax, 0x7172737475767778

movq mm0, [rdx + 8 * 0]
movq mm1, [rdx + 8 * 0]
movq mm2, [rdx + 8 * 0]
movq mm3, [rdx + 8 * 0]
movq mm4, [rdx + 8 * 0]
movq mm5, [rdx + 8 * 0]
movq mm6, [rdx + 8 * 0]
movq mm7, [rdx + 8 * 0]

pinsrw mm0, eax, 0
pinsrw mm1, eax, 1
pinsrw mm2, eax, 2
pinsrw mm3, eax, 3
pinsrw mm4, eax, 4
pinsrw mm5, eax, 5
pinsrw mm6, eax, 6
pinsrw mm7, eax, 7

hlt
