%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x20f9b0697cd37844", "0x3b8e6eae8c165248"],
    "XMM1":  ["0x1ed685b8691d35ca", "0x5dae74e0ab7b51e2"],
    "XMM2":  ["0x15dc41a91ea818cc", "0x092363bc321300c5"],
    "XMM3":  ["0x20f9b0697cd37844", "0x3b8e6eae8c165248"],
    "XMM4":  ["0x1ed685b8691d35ca", "0x5dae74e0ab7b51e2"],
    "XMM5":  ["0x15dc41a91ea818cc", "0x092363bc321300c5"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x2bb883523d4f3197
mov [rdx + 8 * 0], rax
mov rax, 0x1246c77764260189
mov [rdx + 8 * 1], rax

mov rax, 0x163add80bc57bef1
mov [rdx + 8 * 2], rax
mov rax, 0x64d615e5b405a306
mov [rdx + 8 * 3], rax

mov rax, 0x11f4881d94eb39fc
mov [rdx + 8 * 4], rax
mov rax, 0xa9162248f2d0a23a
mov [rdx + 8 * 5], rax

mov rax, 0x0
mov [rdx + 8 * 6], rax
mov rax, 0x0
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

pavgw xmm0, xmm6
pavgw xmm1, xmm7
pavgw xmm2, xmm8

pavgw xmm3, [rdx + 8 * 2]
pavgw xmm4, [rdx + 8 * 4]
pavgw xmm5, [rdx + 8 * 6]

hlt
