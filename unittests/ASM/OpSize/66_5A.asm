%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x404000003F800000", "0x0"],
    "XMM1": ["0x3FF0000000000000", "0x4008000000000000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3FF0000000000000
mov [rdx + 8 * 0], rax
mov rax, 0x4008000000000000
mov [rdx + 8 * 1], rax

mov rax, 0xFFFFFFFFFFFFFFFF
mov [rdx + 8 * 2], rax
mov rax, 0xFFFFFFFFFFFFFFFF
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx + 8 * 2]
movapd xmm1, [rdx]

cvtpd2ps xmm0, xmm1

hlt
