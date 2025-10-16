%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "R9": "1",
    "XMM0":  ["0x4142434400000000", "0xEEEEEEEEEEEEEEEE", "0x0000000000000000", "0x0000000000000000"],
    "XMM1":  ["0x4142434400000000", "0xDDDDDDDDDDDDDDDD", "0x0000000000000000", "0x0000000000000000"],
    "XMM2":  ["0x4142434400000000", "0xCCCCCCCCCCCCCCCC", "0x0000000000000000", "0x0000000000000000"],
    "XMM3":  ["0x4142434400000000", "0xBBBBBBBBBBBBBBBB", "0x0000000000000000", "0x0000000000000000"],
    "XMM4":  ["0x4142434400000000", "0xAAAAAAAAAAAAAAAA", "0x0000000000000000", "0x0000000000000000"],
    "XMM5":  ["0x4142434400000000", "0xAAAAAAAAAAAAAAAA", "0x0000000000000000", "0x0000000000000000"],
    "XMM6":  ["0x4142434400000000", "0xAAAAAAAAAAAAAAAA", "0x0000000000000000", "0x0000000000000000"],
    "XMM7":  ["0x4142434400000000", "0xAAAAAAAAAAAAAAAA", "0x0000000000000000", "0x0000000000000000"],
    "XMM8":  ["0x4142434400000000", "0xDDDDDDDDDDDDDDDD", "0x0000000000000000", "0x0000000000000000"],
    "XMM9":  ["0x4142434400000000", "0xCCCCCCCCCCCCCCCC", "0x0000000000000000", "0x0000000000000000"],
    "XMM10": ["0x4142434400000000", "0xBBBBBBBBBBBBBBBB", "0x0000000000000000", "0x0000000000000000"],
    "XMM11": ["0x4142434400000000", "0xAAAAAAAAAAAAAAAA", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

section .text
global _start

%include "checkprecision.mac"

; This test checks that:
; - the results of the reciprocal sqrt is within 1.5*2^-12 error margin.
; - the top 128 bits of ymms registers are zero.
; - bits [127:32] are correctly copied from the first argument to vrsqrtss.

_start:
vmovapd ymm0, [rel arg1]
vmovapd ymm1, [rel arg2]
vmovapd ymm2, [rel arg3]
vmovapd ymm3, [rel arg4]
vmovapd ymm4, [rel arg5]
vmovapd ymm5, [rel arg5]
vmovapd ymm6, [rel arg5]
vmovapd ymm7, [rel arg5]

; Same register
vrsqrtss xmm0, xmm0, xmm0
vrsqrtss xmm1, xmm1, xmm1
vrsqrtss xmm2, xmm2, xmm2
vrsqrtss xmm3, xmm3, xmm3

; Memory operand
vrsqrtss xmm4, xmm4, [rel arg1]
vrsqrtss xmm5, xmm5, [rel arg2]
vrsqrtss xmm6, xmm6, [rel arg3]
vrsqrtss xmm7, xmm7, [rel arg4]

; Memory operand different source register
vrsqrtss xmm8, xmm1, [rel arg1]
vrsqrtss xmm9, xmm2, [rel arg2]
vrsqrtss xmm10, xmm3, [rel arg3]
vrsqrtss xmm11, xmm4, [rel arg4]

; Check precision
vpextrd [rel result1], xmm0, 0
check_relerr rel eresult1, rel result1, rel tolerance
mov r9, rax

vpextrd [rel result2], xmm1, 0
check_relerr rel eresult2, rel result2, rel tolerance
and r9, rax

vpextrd [rel result3], xmm2, 0
check_relerr rel eresult3, rel result3, rel tolerance
and r9, rax

vpextrd [rel result4], xmm3, 0
check_relerr rel eresult4, rel result4, rel tolerance
and r9, rax

vpextrd [rel result1], xmm8, 0
check_relerr rel eresult1, rel result1, rel tolerance
and r9, rax

vpextrd [rel result2], xmm9, 0
check_relerr rel eresult2, rel result2, rel tolerance
and r9, rax

vpextrd [rel result3], xmm10, 0
check_relerr rel eresult3, rel result3, rel tolerance
and r9, rax

vpextrd [rel result4], xmm11, 0
check_relerr rel eresult4, rel result4, rel tolerance
and r9, rax

; Insert 0s in the bottom 32bits.
xor rax, rax
vpinsrd xmm0, xmm0, eax, 0
vpinsrd xmm1, xmm1, eax, 0
vpinsrd xmm2, xmm2, eax, 0
vpinsrd xmm3, xmm3, eax, 0
vpinsrd xmm4, xmm4, eax, 0
vpinsrd xmm5, xmm5, eax, 0
vpinsrd xmm6, xmm6, eax, 0
vpinsrd xmm7, xmm7, eax, 0
vpinsrd xmm8, xmm8, eax, 0
vpinsrd xmm9, xmm9, eax, 0
vpinsrd xmm10, xmm10, eax, 0
vpinsrd xmm11, xmm11, eax, 0

hlt

align 4096
result1: times 2 dq 0
result2: times 2 dq 0
result3: times 2 dq 0
result4: times 2 dq 0

align 32
arg1:
dq 0x414243443F800000 ; 1.0
dq 0xEEEEEEEEEEEEEEEE
dq 0x5152535455565758
dq 0x5152535455565758

arg2:
dq 0x4142434440800000 ; 4.0
dq 0xDDDDDDDDDDDDDDDD
dq 0x5152535455565758
dq 0x5152535455565758

arg3:
dq 0x4142434441100000 ; 9.0
dq 0xCCCCCCCCCCCCCCCC
dq 0x5152535455565758
dq 0x5152535455565758

arg4:
dq 0x4142434441800000 ; 16.0
dq 0xBBBBBBBBBBBBBBBB
dq 0x5152535455565758
dq 0x5152535455565758

arg5:
dq 0x4142434441C80000 ; 25.0
dq 0xAAAAAAAAAAAAAAAA
dq 0x5152535455565758
dq 0x5152535455565758

eresult1:
dd 0x3F800000 ; 1.0

eresult2:
dd 0x3f000000 ; 0.5

eresult3:
dd 0x3eaaaaab ; 1/3 = 0.(3)

eresult4:
dd 0x3e800000 ; 0.25

tolerance:
dd 0x39c00000

define_check_data_constants
