%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x000000000003FFFC", "0x000000000000FFFE"],
    "XMM1": ["0x000000000003FFFC", "0x000000000000FFFE"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x414243440000FFFF
mov [rdx + 8 * 0], rax
mov rax, 0x5152535400007FFF
mov [rdx + 8 * 1], rax

mov rax, 0x6162636400000004
mov [rdx + 8 * 2], rax
mov rax, 0x7172737400000002
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx]
pmuludq xmm0, [rdx + 8 * 2]

movapd xmm1, [rdx]
movapd xmm2, [rdx + 8 * 2]
pmuludq xmm1, xmm2

hlt
