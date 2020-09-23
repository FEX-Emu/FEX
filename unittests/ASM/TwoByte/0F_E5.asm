%ifdef CONFIG
{
  "RegData": {
    "MM0": "0xD7D1D77A17171851"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x8182838445464748 ; -32382, -31868, 17734, 18248
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758 ; 20818, 21332, 21846, 22360
mov [rdx + 8 * 1], rax

mov rax, 0x6162636465666768
mov [rdx + 8 * 2], rax
mov rax, 0x7172737475767778
mov [rdx + 8 * 3], rax

mov rax, 0x0
mov [rdx + 8 * 4], rax
mov [rdx + 8 * 5], rax

mov eax, dword [rdx + 8 * 0]
mov rbx, qword [rdx + 8 * 1]

movq mm0, [rdx + 8 * 0]

pmulhw mm0, [rdx + 8 * 1]

hlt
