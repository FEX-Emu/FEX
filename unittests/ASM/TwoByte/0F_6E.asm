%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x0000000045464748",
    "MM1": "0x5152535455565758",
    "MM2": "0x0000000045464748",
    "MM3": "0x5152535455565758"
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

mov rax, 0x6162636465666768
mov [rdx + 8 * 2], rax
mov rax, 0x7172737475767778
mov [rdx + 8 * 3], rax

mov rax, 0x0
mov [rdx + 8 * 4], rax
mov [rdx + 8 * 5], rax

mov rax, qword [rdx + 8 * 0]
mov rbx, qword [rdx + 8 * 1]

movd mm0, eax
movq mm1, rbx

movd mm2, dword [rdx + 8 * 0]
movq mm3, qword [rdx + 8 * 1]

hlt
