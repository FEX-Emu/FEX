%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x2179b0697d5378c4", "0x3b8e6eae8c165248"],
    "XMM1":  ["0x1ed68638699d35ca", "0x5e2e7560ab7b5262"],
    "XMM2":  ["0x165c42291f28194c", "0x0923643c32130145"],
    "XMM3":  ["0x2179b0697d5378c4", "0x3b8e6eae8c165248"],
    "XMM4":  ["0x1ed68638699d35ca", "0x5e2e7560ab7b5262"],
    "XMM5":  ["0x165c42291f28194c", "0x0923643c32130145"]
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

pavgb xmm0, xmm6
pavgb xmm1, xmm7
pavgb xmm2, xmm8

pavgb xmm3, [rdx + 8 * 2]
pavgb xmm4, [rdx + 8 * 4]
pavgb xmm5, [rdx + 8 * 6]

hlt
