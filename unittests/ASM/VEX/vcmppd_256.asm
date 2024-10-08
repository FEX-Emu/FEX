%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM2":  ["0xFFFFFFFFFFFFFFFF", "0x0000000000000000", "0xFFFFFFFFFFFFFFFF", "0x0000000000000000"],
    "XMM3":  ["0x0000000000000000", "0xFFFFFFFFFFFFFFFF", "0x0000000000000000", "0xFFFFFFFFFFFFFFFF"],
    "XMM4":  ["0xFFFFFFFFFFFFFFFF", "0xFFFFFFFFFFFFFFFF", "0xFFFFFFFFFFFFFFFF", "0xFFFFFFFFFFFFFFFF"],
    "XMM5":  ["0x0000000000000000", "0xFFFFFFFFFFFFFFFF", "0x0000000000000000", "0xFFFFFFFFFFFFFFFF"],
    "XMM6":  ["0xFFFFFFFFFFFFFFFF", "0x0000000000000000", "0xFFFFFFFFFFFFFFFF", "0x0000000000000000"],
    "XMM7":  ["0x0000000000000000", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM10": ["0xFFFFFFFFFFFFFFFF", "0xFFFFFFFFFFFFFFFF", "0xFFFFFFFFFFFFFFFF", "0xFFFFFFFFFFFFFFFF"],
    "XMM11": ["0x0000000000000000", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM12": ["0x0000000000000000", "0xFFFFFFFFFFFFFFFF", "0x0000000000000000", "0xFFFFFFFFFFFFFFFF"],
    "XMM13": ["0xFFFFFFFFFFFFFFFF", "0x0000000000000000", "0xFFFFFFFFFFFFFFFF", "0x0000000000000000"]
  }
}
%endif

lea rdx, [rel .data]

vmovapd ymm0, [rdx + 32 * 0]
vmovapd ymm1, [rdx + 32 * 1]

vcmppd ymm2, ymm0, ymm1, 0x00 ; EQ
vcmppd ymm3, ymm0, ymm1, 0x01 ; LT
vcmppd ymm4, ymm0, ymm1, 0x02 ; LTE
vcmppd ymm5, ymm0, ymm1, 0x04 ; NEQ
vcmppd ymm6, ymm0, ymm1, 0x05 ; NLT
vcmppd ymm7, ymm0, ymm1, 0x06 ; NLTE

; Unordered and Ordered tests need to be special cased
vmovapd ymm8, [rdx + 32 * 2]
vmovapd ymm9, [rdx + 32 * 3]

; Unordered will return true when either input is nan
; [0.0, nan] unord [nan, 0.0] = [1, 1]
vcmppd ymm10, ymm8, ymm9, 0x03 ; Unordered

; Ordered will return true when both inputs are NOT nan
; [0.0, nan] ord [nan, 0.0] = [0, 0]
vcmppd ymm11, ymm8, ymm9, 0x07 ; Ordered

; Ordered will return true when both inputs are NOT nan
; [nan, 0.0] ord [nan, 0.0] = [0, 1]
vcmppd ymm12, ymm9, ymm9, 0x07 ; Ordered

; Ordered will return true when both inputs are NOT nan
; [0.0, nan] ord [0.0, nan] = [1, 0]
vcmppd ymm13, ymm8, ymm8, 0x07 ; Ordered

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
