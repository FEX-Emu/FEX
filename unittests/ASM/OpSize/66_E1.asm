%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x4142434445464748", "0x7172737475767778"],
    "XMM1":  ["0x0041004300450047", "0x0071007300750077"],
    "XMM2":  ["0x0", "0x0"],
    "XMM3":  ["0x4142434445464748", "0x7172737475767778"],
    "XMM4":  ["0x0041004300450047", "0x0071007300750077"],
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

mov rax, 0x8
mov [rdx + 8 * 4], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 5], rax

mov rax, 0x10
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

psraw xmm0, xmm6
psraw xmm1, xmm7
psraw xmm2, xmm8

psraw xmm3, [rdx + 8 * 2]
psraw xmm4, [rdx + 8 * 4]
psraw xmm5, [rdx + 8 * 6]

hlt
