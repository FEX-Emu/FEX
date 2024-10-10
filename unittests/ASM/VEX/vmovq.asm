%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0x4142434445464748", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM1": ["0x4142434445464748", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM2": ["0x4142434445464748", "0x5152535455565758", "0x4142434445464748", "0x7172737475767778"]
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

; Load from memory (fill with junk and ensure load zeroes out upper 64-bit lanes)
vmovapd xmm0, [rdx]
vmovq xmm0, [rdx]

; Load and truncate same register
vmovapd ymm1, [rdx]
vmovq xmm1, xmm1

; Store and reload
vmovq [rdx + 8 * 2], xmm1
vmovapd ymm2, [rdx]

hlt
