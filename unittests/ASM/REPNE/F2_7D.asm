%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0xc0800000bf800000", "0xc2800000c1800000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x400000003f800000 ; 2.0, 1.0
mov [rdx + 8 * 0], rax
mov rax, 0x4100000040800000 ; 8.0, 4.0
mov [rdx + 8 * 1], rax

mov rax, 0x4200000041800000 ; 32.0, 16.0
mov [rdx + 8 * 2], rax
mov rax, 0x4300000042800000 ; 128.0, 64.0
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx]
movapd xmm1, [rdx + 8 * 2]

hsubps xmm0, xmm1

hlt
