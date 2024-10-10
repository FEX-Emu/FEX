%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0x6162636465666768", "0x7172737475767778", "0x0000000000000000", "0x0000000000000000"],
    "XMM3": ["0x4142434445464748", "0x5152535455565758", "0x6162636465666768", "0x7172737475767778"]
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
mov rax, 0x7172737475767778
mov [rdx + 8 * 3], rax

mov rax, 0x0
mov [rdx + 8 * 4], rax
mov [rdx + 8 * 5], rax
mov [rdx + 8 * 6], rax
mov [rdx + 8 * 7], rax

vmovaps xmm0, [rdx + 8 * 0]
vmovaps xmm1, [rdx + 8 * 2]
vmovaps ymm2, [rdx + 8 * 0]

vmovntps [rdx + 8 * 4], xmm1
vmovaps xmm0, [rdx + 8 * 4]

vmovntps [rdx + 8 * 4], ymm2
vmovaps ymm3, [rdx + 8 * 4]

hlt
