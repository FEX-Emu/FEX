%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x1111111111111111", "0x2222222222222222"],
    "XMM1": ["0x1111111111111111", "0x2222222222222222"],
    "XMM2": ["0x0101010101010101", "0x0202020202020202"]
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

movapd xmm0, [rdx]
por xmm0, [rdx + 8 * 2]

movapd xmm1, [rdx]
movapd xmm2, [rdx + 8 * 2]
por xmm1, xmm2

hlt
