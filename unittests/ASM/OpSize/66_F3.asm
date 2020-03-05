%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x4142434445464748", "0x7172737475767778"],
    "XMM1":  ["0x4546474800000000", "0x7576777800000000"],
    "XMM2":  ["0x0", "0x0"],
    "XMM3":  ["0x4142434445464748", "0x7172737475767778"],
    "XMM4":  ["0x4546474800000000", "0x7576777800000000"],
    "XMM5":  ["0x0", "0x0"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x7172737475767778
mov [rdx + 8 * 1], rax

mov rax, 0x0
mov [rdx + 8 * 2], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 3], rax

mov rax, 0x20
mov [rdx + 8 * 4], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 5], rax

mov rax, 0x40
mov [rdx + 8 * 6], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 7], rax

movapd xmm0, [rdx + 8 * 0]
movapd xmm1, [rdx + 8 * 0]
movapd xmm2, [rdx + 8 * 0]
movapd xmm3, [rdx + 8 * 0]
movapd xmm4, [rdx + 8 * 0]
movapd xmm5, [rdx + 8 * 0]

movapd xmm6, [rdx + 8 * 2]
movapd xmm7, [rdx + 8 * 4]
movapd xmm8, [rdx + 8 * 6]

psllq xmm0, xmm6
psllq xmm1, xmm7
psllq xmm2, xmm8

psllq xmm3, [rdx + 8 * 2]
psllq xmm4, [rdx + 8 * 4]
psllq xmm5, [rdx + 8 * 6]

hlt
