%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "RBX": "0x4142434445464748",
    "RCX": "0x0000000045464748",
    "XMM0": ["0x7172737475767778", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM1": ["0x0000000075767778", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM2": ["0x0000000045464748", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM3": ["0x4142434445464748", "0x5152535455565758", "0x0000000000000000", "0x0000000000000000"],
    "XMM4": ["0x6162636465666768", "0x7172737475767778", "0x0000000000000000", "0x0000000000000000"],
    "XMM5": ["0x4142434465666768", "0x6162636465666768", "0x6162636465666768", "0x7172737475767778"]
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

; Load from GPR (64-bit)
vmovapd xmm0, [rdx]
vmovq xmm0, rax

; Load from GPR (32-bit)
vmovapd xmm1, [rdx]
vmovd xmm1, eax

; Load 32-bit value
vmovapd xmm2, [rdx]
vmovd xmm2, [edx]

; Store into GPR
vmovapd xmm3, [rdx]
vmovq rbx, xmm3
vmovd ecx, xmm3

; Store into mem
vmovapd xmm4, [rdx + 8 * 2]
vmovd [rdx + 0], xmm4
vmovq [rdx + 8], xmm4
vmovapd ymm5, [rdx]

hlt
