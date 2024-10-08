%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM2":  ["0xFFFFFFFFFFFFFFFF", "0x4000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM3":  ["0x0000000000000000", "0x4000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM4":  ["0xFFFFFFFFFFFFFFFF", "0x4000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM5":  ["0x0000000000000000", "0x4000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM6":  ["0xFFFFFFFFFFFFFFFF", "0x4000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM7":  ["0x0000000000000000", "0x4000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM10": ["0xFFFFFFFFFFFFFFFF", "0x7FF8000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM11": ["0x0000000000000000", "0x7FF8000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM12": ["0x0000000000000000", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM13": ["0xFFFFFFFFFFFFFFFF", "0x7FF8000000000000", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

lea rdx, [rel .data]

vmovapd ymm0, [rdx + 32 * 0]
vmovapd ymm1, [rdx + 32 * 1]

vcmpsd xmm2, xmm0, xmm1, 0x00 ; EQ
vcmpsd xmm3, xmm0, xmm1, 0x01 ; LT
vcmpsd xmm4, xmm0, xmm1, 0x02 ; LTE
vcmpsd xmm5, xmm0, xmm1, 0x04 ; NEQ
vcmpsd xmm6, xmm0, xmm1, 0x05 ; NLT
vcmpsd xmm7, xmm0, xmm1, 0x06 ; NLTE

; Unordered and Ordered tests need to be special cased
vmovapd ymm8, [rdx + 32 * 2]
vmovapd ymm9, [rdx + 32 * 3]

; Unordered will return true when either input is nan
; [0.0, nan] unord [nan, 0.0] = [1, 1]
vcmpsd xmm10, xmm8, xmm9, 0x03 ; Unordered

; Ordered will return true when both inputs are NOT nan
; [0.0, nan] ord [nan, 0.0] = [0, 0]
vcmpsd xmm11, xmm8, xmm9, 0x07 ; Ordered

; Ordered will return true when both inputs are NOT nan
; [nan, 0.0] ord [nan, 0.0] = [0, 1]
vcmpsd xmm12, xmm9, xmm8, 0x07 ; Ordered

; Ordered will return true when both inputs are NOT nan
; [0.0, nan] ord [0.0, nan] = [1, 0]
vcmpsd xmm13, xmm8, xmm8, 0x07 ; Ordered

hlt

align 32
.data:
dq 0x3FF0000000000000
dq 0x4000000000000000
dq 0x3FF0000000000000
dq 0x4000000000000000

dq 0x3FF0000000000000
dq 0x4008000000000000
dq 0x3FF0000000000000
dq 0x4008000000000000

dq 0x0000000000000000
dq 0x7FF8000000000000
dq 0x0000000000000000
dq 0x7FF8000000000000

dq 0x7FF8000000000000
dq 0x0000000000000000
dq 0x7FF8000000000000
dq 0x0000000000000000
