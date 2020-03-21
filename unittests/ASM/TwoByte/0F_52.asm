%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x3f8000003f800000", "0x3f8000003f800000"],
    "XMM1":  ["0x3f0000003f000000", "0x3f0000003f000000"],
    "XMM2":  ["0x3eaaaaab3eaaaaab", "0x3eaaaaab3eaaaaab"],
    "XMM3":  ["0x3e8000003e800000", "0x3e8000003e800000"],
    "XMM4":  ["0x3f8000003f800000", "0x3f8000003f800000"],
    "XMM5":  ["0x3f0000003f000000", "0x3f0000003f000000"],
    "XMM6":  ["0x3eaaaaab3eaaaaab", "0x3eaaaaab3eaaaaab"],
    "XMM7":  ["0x3e8000003e800000", "0x3e8000003e800000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3f8000003f800000 ; 1.0
mov [rdx + 8 * 0], rax
mov rax, 0x3f8000003f800000
mov [rdx + 8 * 1], rax

mov rax, 0x4080000040800000 ; 4.0
mov [rdx + 8 * 2], rax
mov rax, 0x4080000040800000
mov [rdx + 8 * 3], rax

mov rax, 0x4110000041100000 ; 9.0
mov [rdx + 8 * 4], rax
mov rax, 0x4110000041100000
mov [rdx + 8 * 5], rax

mov rax, 0x4180000041800000 ; 16.0
mov [rdx + 8 * 6], rax
mov rax, 0x4180000041800000
mov [rdx + 8 * 7], rax

mov rax, 0x41c8000041c80000 ; 25.0
mov [rdx + 8 * 8], rax
mov rax, 0x41c8000041c80000
mov [rdx + 8 * 9], rax

movapd xmm0, [rdx + 8 * 0]
movapd xmm1, [rdx + 8 * 2]
movapd xmm2, [rdx + 8 * 4]
movapd xmm3, [rdx + 8 * 6]
movapd xmm4, [rdx + 8 * 8]
movapd xmm5, [rdx + 8 * 8]
movapd xmm6, [rdx + 8 * 8]
movapd xmm7, [rdx + 8 * 8]

rsqrtps xmm0, xmm0
rsqrtps xmm1, xmm1
rsqrtps xmm2, xmm2
rsqrtps xmm3, xmm3

rsqrtps xmm4, [rdx + 8 * 0]
rsqrtps xmm5, [rdx + 8 * 2]
rsqrtps xmm6, [rdx + 8 * 4]
rsqrtps xmm7, [rdx + 8 * 6]

hlt
