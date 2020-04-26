%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x4008000000000000", "0x4028000000000000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3ff0000000000000; 1.0
mov [rdx + 8 * 0], rax
mov rax, 0x4000000000000000 ; 2.0
mov [rdx + 8 * 1], rax

mov rax, 0x4010000000000000 ; 4
mov [rdx + 8 * 2], rax
mov rax, 0x4020000000000000 ; 8
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx]
movapd xmm1, [rdx + 8 * 2]

haddpd xmm0, xmm1

hlt
