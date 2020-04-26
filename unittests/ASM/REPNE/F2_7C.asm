%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x4140000040400000", "0x4340000042400000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3f80000040000000 ; 1.0, 2.0
mov [rdx + 8 * 0], rax
mov rax, 0x4080000041000000 ; 4.0, 8.0
mov [rdx + 8 * 1], rax

mov rax, 0x4180000042000000 ; 16.0, 32.0
mov [rdx + 8 * 2], rax
mov rax, 0x4280000043000000 ; 64.0, 128.0
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx]
movapd xmm1, [rdx + 8 * 2]

haddps xmm0, xmm1

hlt
