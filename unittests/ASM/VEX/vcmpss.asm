%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM2":  ["0x51525354FFFFFFFF", "0x5152535440000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM3":  ["0x5152535400000000", "0x5152535440000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM4":  ["0x51525354FFFFFFFF", "0x5152535440000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM5":  ["0x5152535400000000", "0x5152535440000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM6":  ["0x51525354FFFFFFFF", "0x5152535440000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM7":  ["0x5152535400000000", "0x5152535440000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM10": ["0x0000000000000000", "0x7FC000007FC00000", "0x0000000000000000", "0x0000000000000000"],
    "XMM11": ["0x00000000FFFFFFFF", "0x7FC000007FC00000", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

lea rdx, [rel .data]

vmovapd ymm0, [rdx + 32 * 0]
vmovapd ymm1, [rdx + 32 * 1]

vcmpss xmm2, xmm0, xmm1, 0x00 ; EQ
vcmpss xmm3, xmm0, xmm1, 0x01 ; LT
vcmpss xmm4, xmm0, xmm1, 0x02 ; LTE
vcmpss xmm5, xmm0, xmm1, 0x04 ; NEQ
vcmpss xmm6, xmm0, xmm1, 0x05 ; NLT
vcmpss xmm7, xmm0, xmm1, 0x06 ; NLTE

; Unordered and Ordered tests need to be special cased
vmovapd ymm8, [rdx + 32 * 2]
vmovapd ymm9, [rdx + 32 * 3]

; Unordered will return true when either input is nan
; [0.0, 0.0, nan, nan] unord [0.0, nan, 0.0, nan] = [0, 1, 1, 1]
vcmpss xmm10, xmm8, xmm9, 0x03 ; Unordered

; Ordered will return true when both inputs are NOT nan
; [0.0, 0.0, nan, nan] ord [0.0, nan, 0.0, nan] = [1, 0, 0, 0]
vcmpss xmm11, xmm8, xmm9, 0x07 ; Ordered

hlt

align 32
.data:
dq 0x515253543F800000
dq 0x5152535440000000
dq 0x515253543F800000
dq 0x5152535440000000

dq 0x515253543F800000
dq 0x5152535440800000
dq 0x515253543F800000
dq 0x5152535440800000

dq 0x0000000000000000
dq 0x7FC000007FC00000
dq 0x0000000000000000
dq 0x7FC000007FC00000

dq 0x7FC0000000000000
dq 0x7FC0000000000000
dq 0x7FC0000000000000
dq 0x7FC0000000000000
