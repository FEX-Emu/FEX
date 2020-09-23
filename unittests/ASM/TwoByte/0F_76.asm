%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x00000000FFFFFFFF",
    "MM1": "0x00000000FFFFFFFF",
    "MM2": "0x61626364FFFFFFFF"
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

movq mm0, [rdx]
pcmpeqd mm0, [rdx + 8 * 2]

movq mm1, [rdx]
movq mm2, [rdx + 8 * 2]
pcmpeqd mm1, mm2

hlt
