%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x2",
    "XMM0": ["0x0", "0x8000000000000000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x0
mov [rdx + 8 * 0], rax
mov rax, 0x8000000000000000
mov [rdx + 8 * 1], rax

movapd xmm0, [rdx]
movmskpd rax, xmm0

hlt
