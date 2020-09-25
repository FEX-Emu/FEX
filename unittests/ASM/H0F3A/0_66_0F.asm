%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x7861626364656667", "0x4871727374757677"],
    "XMM2": ["0x7861626364656667", "0x4871727374757677"],
    "XMM3": ["0x5354555657584142", "0x0000000000005152"],
    "XMM4": ["0x0", "0x0"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

mov rax, 0x6162636465666768
mov [rdx + 8 * 2], rax
mov rax, 0x7172737475767778
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx]
movapd xmm1, [rdx + 16]

palignr xmm0, xmm1, 1

movapd xmm2, [rdx]
movapd xmm3, [rdx + 16]

db 0x48 ; Glues Rex.W to the start of the instruction
palignr xmm2, xmm3, 1

movapd xmm3, [rdx]
movapd xmm4, [rdx + 16]

palignr xmm3, xmm4, 22

movapd xmm4, [rdx]
movapd xmm5, [rdx + 16]

palignr xmm4, xmm5, 32

hlt
