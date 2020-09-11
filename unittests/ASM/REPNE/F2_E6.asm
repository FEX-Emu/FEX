%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x0000000200000001", "0x0"],
    "XMM1":  ["0xFFFFFFFEFFFFFFFF", "0x0"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3ff0000000000000
mov [rdx + 8 * 0], rax
mov rax, 0x4000000000000000
mov [rdx + 8 * 1], rax

mov rax, 0xbff0000000000000
mov [rdx + 8 * 2], rax
mov rax, 0xc000000000000000
mov [rdx + 8 * 3], rax

mov rax, 0x4142434445464748
mov [rdx + 8 * 4], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 5], rax

movapd xmm0, [rdx + 8 * 4]
movapd xmm1, [rdx + 8 * 4]

movapd xmm2, [rdx + 8 * 0]

cvtpd2dq xmm0, xmm2
cvtpd2dq xmm1, [rdx + 8 * 2]

hlt
