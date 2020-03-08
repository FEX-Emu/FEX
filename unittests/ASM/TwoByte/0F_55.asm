%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x0", "0xFFFFFFFFFFFFFFFF"],
    "XMM1": ["0x0", "0xFFFFFFFFFFFFFFFF"],
    "XMM2": ["0x1010101010101010", "0xFFFFFFFFFFFFFFFF"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0xFFFFFFFFFFFFFFFF
mov [rdx + 8 * 0], rax
mov rax, 0x0
mov [rdx + 8 * 1], rax

mov rax, 0x1010101010101010
mov [rdx + 8 * 2], rax
mov rax, 0xFFFFFFFFFFFFFFFF
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx]
andnps xmm0, [rdx + 8 * 2]

movapd xmm1, [rdx]
movapd xmm2, [rdx + 8 * 2]
andnps xmm1, xmm2

hlt
