%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0xbff0000000000000", "0x4008000000000000"],
    "XMM1": ["0xbff0000000000000", "0x4008000000000000"],
    "XMM2": ["0x3ff0000000000000", "0x4008000000000000"],
    "XMM3": ["0x3ff0000000000000", "0x4008000000000000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3FF0000000000000
mov [rdx + 8 * 0], rax
mov rax, 0x3FF0000000000000
mov [rdx + 8 * 1], rax

mov rax, 0x4000000000000000
mov [rdx + 8 * 2], rax
mov rax, 0x4000000000000000
mov [rdx + 8 * 3], rax

mov rax, 0x4000000000000000
mov [rdx + 8 * 4], rax
mov rax, 0x4000000000000000
mov [rdx + 8 * 5], rax

mov rax, 0x3FF0000000000000
mov [rdx + 8 * 6], rax
mov rax, 0x3FF0000000000000
mov [rdx + 8 * 7], rax

movapd xmm0, [rdx]
addsubpd xmm0, [rdx + 8 * 2]

movapd xmm1, [rdx]
movapd xmm2, [rdx + 8 * 2]
addsubpd xmm1, xmm2

movapd xmm2, [rdx + 8 * 4]
addsubpd xmm2, [rdx + 8 * 6]

movapd xmm3, [rdx + 8 * 4]
movapd xmm4, [rdx + 8 * 6]
addsubpd xmm3, xmm4

hlt
