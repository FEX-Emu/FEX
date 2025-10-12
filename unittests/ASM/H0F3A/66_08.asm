%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x8000000000000000", "0xc000000040000000"],
    "XMM1": ["0xbf80000000000000", "0xc00000003f800000"],
    "XMM2": ["0x800000003f800000", "0xbf80000040000000"],
    "XMM3": ["0x8000000000000000", "0xbf8000003f800000"],
    "XMM4": ["0x8000000000000000", "0xc000000040000000"],
    "XMM5": ["0xbf80000000000000", "0xc00000003f800000"],
    "XMM6": ["0x800000003f800000", "0xbf80000040000000"],
    "XMM7": ["0x8000000000000000", "0xbf8000003f800000"]
  },
  "HostFeatures": ["SSE4.1"]
}
%endif

lea rdx, [rel .data]

roundps xmm0, [rdx + 8 * 0], 00000000b ; Nearest
roundps xmm1, [rdx + 8 * 0], 00000001b ; -inf
roundps xmm2, [rdx + 8 * 0], 00000010b ; +inf
roundps xmm3, [rdx + 8 * 0], 00000011b ; truncate

; MXCSR
; Set to nearest
mov eax, 0x1F80
mov [rdx + 8 * 2], eax
ldmxcsr [rdx + 8 * 2]

roundps xmm4, [rdx + 8 * 0], 00000100b

; Set to -inf
mov eax, 0x3F80
mov [rdx + 8 * 2], eax
ldmxcsr [rdx + 8 * 2]

roundps xmm5, [rdx + 8 * 0], 00000100b

; Set to +inf
mov eax, 0x5F80
mov [rdx + 8 * 2], eax
ldmxcsr [rdx + 8 * 2]

roundps xmm6, [rdx + 8 * 0], 00000100b

; Set to truncate
mov eax, 0x7F80
mov [rdx + 8 * 2], eax
ldmxcsr [rdx + 8 * 2]

roundps xmm7, [rdx + 8 * 0], 00000100b

hlt

align 4096
.data:
dd 0.5, -0.5, 1.5, -1.5
dq 0, 0
