%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x3ff0000000000000", "0x4000000000000000"],
    "XMM1":  ["0x4008000000000000", "0x4010000000000000"]
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

mov rax, 0x0000000200000001
mov [rdx + 8 * 2], rax
mov rax, 0x0000000400000003
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx + 8 * 0]
movapd xmm1, [rdx + 8 * 0]

movapd xmm2, [rdx + 8 * 2]

cvtdq2pd xmm0, xmm2
cvtdq2pd xmm1, [rdx + 8 * 3]

hlt
