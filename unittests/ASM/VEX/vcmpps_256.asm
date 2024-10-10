%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM2":  ["0xFFFFFFFFFFFFFFFF", "0x0000000000000000", "0xFFFFFFFFFFFFFFFF", "0x0000000000000000"],
    "XMM3":  ["0x0000000000000000", "0xFFFFFFFF00000000", "0x0000000000000000", "0xFFFFFFFF00000000"],
    "XMM4":  ["0xFFFFFFFFFFFFFFFF", "0xFFFFFFFF00000000", "0xFFFFFFFFFFFFFFFF", "0xFFFFFFFF00000000"],
    "XMM5":  ["0x0000000000000000", "0xFFFFFFFFFFFFFFFF", "0x0000000000000000", "0xFFFFFFFFFFFFFFFF"],
    "XMM6":  ["0xFFFFFFFFFFFFFFFF", "0x00000000FFFFFFFF", "0xFFFFFFFFFFFFFFFF", "0x00000000FFFFFFFF"],
    "XMM7":  ["0x0000000000000000", "0x00000000FFFFFFFF", "0x0000000000000000", "0x00000000FFFFFFFF"],
    "XMM10": ["0xFFFFFFFF00000000", "0xFFFFFFFFFFFFFFFF", "0xFFFFFFFF00000000", "0xFFFFFFFFFFFFFFFF"],
    "XMM11": ["0x00000000FFFFFFFF", "0x0000000000000000", "0x00000000FFFFFFFF", "0x0000000000000000"]
  }
}
%endif

lea rdx, [rel .data]

vmovapd ymm0, [rdx + 32 * 0]
vmovapd ymm1, [rdx + 32 * 1]

vcmpps ymm2, ymm0, ymm1, 0x00 ; EQ
vcmpps ymm3, ymm0, ymm1, 0x01 ; LT
vcmpps ymm4, ymm0, ymm1, 0x02 ; LTE
vcmpps ymm5, ymm0, ymm1, 0x04 ; NEQ
vcmpps ymm6, ymm0, ymm1, 0x05 ; NLT
vcmpps ymm7, ymm0, ymm1, 0x06 ; NLTE

; Unordered and Ordered tests need to be special cased
vmovapd ymm8, [rdx + 32 * 2]
vmovapd ymm9, [rdx + 32 * 3]

; Unordered will return true when either input is nan
; [0.0, 0.0, nan, nan] unord [0.0, nan, 0.0, nan] = [0, 1, 1, 1]
vcmpps ymm10, ymm8, ymm9, 0x03 ; Unordered

; Ordered will return true when both inputs are NOT nan
; [0.0, 0.0, nan, nan] ord [0.0, nan, 0.0, nan] = [1, 0, 0, 0]
vcmpps ymm11, ymm8, ymm9, 0x07 ; Ordered

hlt

align 32
.data:
dq 0x3F80000040000000
dq 0x4000000040800000
dq 0x3F80000040000000
dq 0x4000000040800000

dq 0x3F80000040000000
dq 0x40A000003F800000
dq 0x3F80000040000000
dq 0x40A000003F800000

dq 0x0000000000000000
dq 0x7FC000007FC00000
dq 0x0000000000000000
dq 0x7FC000007FC00000

dq 0x7FC0000000000000
dq 0x7FC0000000000000
dq 0x7FC0000000000000
dq 0x7FC0000000000000
