%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x00000000FFFFFFFF", "0x00000000FFFFFFFF"],
    "XMM1": ["0x00000000FFFFFFFF", "0x00000000FFFFFFFF"],
    "XMM2": ["0x61626364FFFFFFFF", "0x51525354FFFFFFFF"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x71727374FFFFFFFF
mov [rdx + 8 * 0], rax
mov rax, 0x41424344FFFFFFFF
mov [rdx + 8 * 1], rax

mov rax, 0x61626364FFFFFFFF
mov [rdx + 8 * 2], rax
mov rax, 0x51525354FFFFFFFF
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx]
pcmpeqd xmm0, [rdx + 8 * 2]

movapd xmm1, [rdx]
movapd xmm2, [rdx + 8 * 2]
pcmpeqd xmm1, xmm2

hlt
