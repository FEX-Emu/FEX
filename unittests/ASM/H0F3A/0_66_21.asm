%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x4142434465666768", "0x5152535455565758"],
    "XMM1":  ["0x4142434461626364", "0x5152535455565758"],
    "XMM2":  ["0x7576777845464748", "0x5152535455565758"],
    "XMM3":  ["0x4142434445464748", "0x5152535471727374"],
    "XMM4":  ["0x4142434445464748", "0x7576777855565758"],
    "XMM5":  ["0x4142434445464748", "0x5152535475767778"],
    "XMM6":  ["0x7576777845464748", "0x5152535455565758"],
    "XMM7":  ["0x4142434475767778", "0x5152535455565758"],
    "XMM8":  ["0x0000000065666768", "0x5152535455565758"],
    "XMM9":  ["0x0000000061626364", "0x5152535455565758"],
    "XMM10": ["0x0000000000000000", "0x0000000000000000"]
  },
  "HostFeatures": ["SSE4.1"]
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

movapd xmm0, [rdx]
movapd xmm1, [rdx]
movapd xmm2, [rdx]
movapd xmm3, [rdx]
movapd xmm4, [rdx]
movapd xmm5, [rdx]
movapd xmm6, [rdx]
movapd xmm7, [rdx]
movapd xmm8, [rdx]
movapd xmm9, [rdx]
movapd xmm10, [rdx]
movapd xmm15, [rdx + 8 * 2]

; Simple move Reg<-Reg
insertps xmm0, xmm15, ((0b00 << 6) | (0b00 << 4) | (0b0000))
insertps xmm1, xmm15, ((0b01 << 6) | (0b00 << 4) | (0b0000))
insertps xmm2, xmm15, ((0b10 << 6) | (0b01 << 4) | (0b0000))
insertps xmm3, xmm15, ((0b11 << 6) | (0b10 << 4) | (0b0000))

; Simple move Reg<-Mem
insertps xmm4, [rdx + 8 * 3], ((0b00 << 6) | (0b11 << 4) | (0b0000))
insertps xmm5, [rdx + 8 * 3], ((0b01 << 6) | (0b10 << 4) | (0b0000))
insertps xmm6, [rdx + 8 * 3], ((0b10 << 6) | (0b01 << 4) | (0b0000))
insertps xmm7, [rdx + 8 * 3], ((0b11 << 6) | (0b00 << 4) | (0b0000))

; Simple move Reg<-Reg with mask
insertps xmm8, xmm15, ((0b00 << 6) | (0b00 << 4) | (0b0010))
insertps xmm9, xmm15, ((0b01 << 6) | (0b00 << 4) | (0b0010))

; Full ZMask
insertps xmm10, xmm15, ((0b00 << 6) | (0b00 << 4) | (0b1111))

hlt
