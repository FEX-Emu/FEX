%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0xbf8000003f800000", "0x437f000000000000"],
    "XMM1": ["0xbf8000003f800000", "0x437f000000000000"],
    "XMM2": ["0xFFFFFFFF00000001", "0x000000FF00000000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x400000003f800000 ; 2, 1
mov [rdx + 8 * 0], rax
mov rax, 0x4080000040400000 ; 4, 3
mov [rdx + 8 * 1], rax

mov rax, 0xFFFFFFFF00000001 ; -1, 1
mov [rdx + 8 * 2], rax
mov rax, 0x000000FF00000000 ; 255, 0
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx]
cvtdq2ps xmm0, [rdx + 8 * 2]

movapd xmm1, [rdx]
movapd xmm2, [rdx + 8 * 2]
cvtdq2ps xmm1, xmm2

hlt
