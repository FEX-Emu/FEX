%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x4142434445464748", "0x5152535455565758"],
    "XMM1": ["0x5152535455565758", "0x6162636465666768"],
    "XMM2": ["0x4142434445464748", "0x5152535455565758"],
    "XMM3": ["0x0", "0x0"],
    "XMM4": ["0x5152535455565758", "0x6162636465666768"]
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

mov rax, 0
mov [rdx + 8 * 3], rax
mov [rdx + 8 * 4], rax
mov [rdx + 8 * 5], rax
mov [rdx + 8 * 6], rax
mov [rdx + 8 * 7], rax

movdqu xmm0, [rdx + 8 * 0]
movdqu xmm1, [rdx + 8 * 1]

movdqu [rdx + 8 * 3], xmm0

movdqu xmm2, [rdx + 8 * 3]
; Ensure it didn't write past where it should
movdqu xmm3, [rdx + 8 * 5]

movdqu xmm4, xmm1

hlt
