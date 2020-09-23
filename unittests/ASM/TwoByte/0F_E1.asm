%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x4142434445464748",
    "MM1": "0x0041004300450047",
    "MM2": "0x0",
    "MM3": "0x4142434445464748",
    "MM4": "0x0041004300450047",
    "MM5": "0x0"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x7172737475767778
mov [rdx + 8 * 1], rax

mov rax, 0x0
mov [rdx + 8 * 2], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 3], rax

mov rax, 0x8
mov [rdx + 8 * 4], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 5], rax

mov rax, 0x10
mov [rdx + 8 * 6], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 7], rax

movq mm0, [rdx + 8 * 0]
movq mm1, [rdx + 8 * 0]
movq mm2, [rdx + 8 * 0]
movq mm3, [rdx + 8 * 0]
movq mm4, [rdx + 8 * 0]
movq mm5, [rdx + 8 * 0]
movq mm6, [rdx + 8 * 2]
movq mm7, [rdx + 8 * 4]

psraw mm0, mm6
psraw mm1, mm7

movq mm7, [rdx + 8 * 6]
psraw mm2, mm7

psraw mm3, [rdx + 8 * 2]
psraw mm4, [rdx + 8 * 4]
psraw mm5, [rdx + 8 * 6]

hlt
