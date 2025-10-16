%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0x0000000000000000", "0xBFE0000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM1": ["0x0000000000000000", "0xBFE0000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM2": ["0x3FF0000000000000", "0xBFE0000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM3": ["0x0000000000000000", "0xBFE0000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM4": ["0x0000000000000000", "0xBFE0000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM5": ["0x0000000000000000", "0xBFE0000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM6": ["0x3FF0000000000000", "0xBFE0000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM7": ["0x0000000000000000", "0xBFE0000000000000", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

lea rdx, [rel .data]

vmovaps ymm0, [rdx]
vmovaps ymm1, [rdx]
vmovaps ymm2, [rdx]
vmovaps ymm3, [rdx]
vmovaps ymm4, [rdx]
vmovaps ymm5, [rdx]
vmovaps ymm6, [rdx]
vmovaps ymm7, [rdx]

vroundsd xmm0, xmm0, [rdx], 00000000b ; Nearest
vroundsd xmm1, xmm1, [rdx], 00000001b ; -inf
vroundsd xmm2, xmm2, [rdx], 00000010b ; +inf
vroundsd xmm3, xmm3, [rdx], 00000011b ; truncate

; MXCSR
; Set to nearest
mov eax, 0x1F80
mov [rel .mxcsr], eax
ldmxcsr [rel .mxcsr]

vroundsd xmm4, xmm4, [rdx], 00000100b

; Set to -inf
mov eax, 0x3F80
mov [rel .mxcsr], eax
ldmxcsr [rel .mxcsr]

vroundsd xmm5, xmm5, [rdx], 00000100b

; Set to +inf
mov eax, 0x5F80
mov [rel .mxcsr], eax
ldmxcsr [rel .mxcsr]

vroundsd xmm6, xmm6, [rdx], 00000100b

; Set to truncate
mov eax, 0x7F80
mov [rel .mxcsr], eax
ldmxcsr [rel .mxcsr]

vroundsd xmm7, xmm7, [rdx], 00000100b

hlt

align 4096
.data:
dq 0.5, -0.5
dq 0.5, -0.5

.mxcsr:
dq 0, 0
