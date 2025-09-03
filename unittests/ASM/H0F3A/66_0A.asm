%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0xbf00000000000000", "0xbfc000003fc00000"],
    "XMM1": ["0xbf00000000000000", "0xbfc000003fc00000"],
    "XMM2": ["0xbf0000003f800000", "0xbfc000003fc00000"],
    "XMM3": ["0xbf00000000000000", "0xbfc000003fc00000"],
    "XMM4": ["0xbf00000000000000", "0xbfc000003fc00000"],
    "XMM5": ["0xbf00000000000000", "0xbfc000003fc00000"],
    "XMM6": ["0xbf0000003f800000", "0xbfc000003fc00000"],
    "XMM7": ["0xbf00000000000000", "0xbfc000003fc00000"]
  },
  "HostFeatures": ["SSE4.1"]
}
%endif

lea rdx, [rel .data]

movaps xmm0, [rdx + 8 * 0]
movaps xmm1, [rdx + 8 * 0]
movaps xmm2, [rdx + 8 * 0]
movaps xmm3, [rdx + 8 * 0]
movaps xmm4, [rdx + 8 * 0]
movaps xmm5, [rdx + 8 * 0]
movaps xmm6, [rdx + 8 * 0]
movaps xmm7, [rdx + 8 * 0]

roundss xmm0, [rdx + 8 * 0], 00000000b ; Nearest
roundss xmm1, [rdx + 8 * 0], 00000001b ; -inf
roundss xmm2, [rdx + 8 * 0], 00000010b ; +inf
roundss xmm3, [rdx + 8 * 0], 00000011b ; truncate

; MXCSR
; Set to nearest
mov eax, 0x1F80
mov [rdx + 8 * 2], eax
ldmxcsr [rdx + 8 * 2]

roundss xmm4, [rdx + 8 * 0], 00000100b

; Set to -inf
mov eax, 0x3F80
mov [rdx + 8 * 2], eax
ldmxcsr [rdx + 8 * 2]

roundss xmm5, [rdx + 8 * 0], 00000100b

; Set to +inf
mov eax, 0x5F80
mov [rdx + 8 * 2], eax
ldmxcsr [rdx + 8 * 2]

roundss xmm6, [rdx + 8 * 0], 00000100b

; Set to truncate
mov eax, 0x7F80
mov [rdx + 8 * 2], eax
ldmxcsr [rdx + 8 * 2]

roundss xmm7, [rdx + 8 * 0], 00000100b

hlt

align 16
.data:
dd 0.5, -0.5, 1.5, -1.5
dq 0, 0
