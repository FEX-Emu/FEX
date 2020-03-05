%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x3ff0000000000000", "0x5152535455565758"],
    "XMM1":  ["0x4000000000000000", "0x5152535455565758"],
    "XMM2":  ["0x4008000000000000", "0x5152535455565758"],
    "XMM3":  ["0x4010000000000000", "0x5152535455565758"]
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

mov rax, 0x1
mov [rdx + 8 * 2], rax
mov rax, 0x2
mov [rdx + 8 * 3], rax
mov rax, 0x3
mov [rdx + 8 * 4], rax
mov rax, 0x4
mov [rdx + 8 * 5], rax

movapd xmm0, [rdx + 8 * 0]
movapd xmm1, [rdx + 8 * 0]
movapd xmm2, [rdx + 8 * 0]
movapd xmm3, [rdx + 8 * 0]

mov rax, [rdx + 8 * 2]
mov rbx, [rdx + 8 * 3]

cvtsi2sd xmm0, rax
cvtsi2sd xmm1, ebx

cvtsi2sd xmm2, dword [rdx + 8 * 4]
cvtsi2sd xmm3, qword [rdx + 8 * 5]

hlt
