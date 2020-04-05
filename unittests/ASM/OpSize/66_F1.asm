%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x828486888A8C8E90", "0xE2E4E6E8EAECEEF0"],
    "XMM1":  ["0x4200440046004800", "0x7200740076007800"],
    "XMM2":  ["0x0", "0x0"],
    "XMM3":  ["0x0", "0x0"]
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

mov rax, 0x1
mov [rdx + 8 * 2], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 3], rax

mov rax, 0x8
mov [rdx + 8 * 4], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 5], rax

mov rax, 0x10
mov [rdx + 8 * 6], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 7], rax

mov rax, 0x20
mov [rdx + 8 * 8], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 9], rax

movaps xmm0, [rdx + 8 * 0]
movaps xmm1, [rdx + 8 * 0]
movaps xmm2, [rdx + 8 * 0]
movaps xmm3, [rdx + 8 * 0]

psllw xmm0, [rdx + 8 * 2]
psllw xmm1, [rdx + 8 * 4]
psllw xmm2, [rdx + 8 * 6]
psllw xmm3, [rdx + 8 * 8]

hlt
