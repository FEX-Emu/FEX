%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "R9": "1",
    "XMM0": ["0x3F8000003F800000", "0x3F8000003F800000", "0x3F8000003F800000", "0x3F8000003F800000"],
    "XMM1": ["0x4080000040800000", "0x4080000040800000", "0x4080000040800000", "0x4080000040800000"],
    "XMM2": ["0x4080000000000000", "0x4080000040800000", "0x0000000000000000", "0x0000000000000000"],
    "XMM3": ["0x3F80000000000000", "0x3F8000003F800000", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

section .text
global _start

%include "checkprecision.mac"

; This test checks that:
; - the results of the reciprocal sqrt is within 1.5*2^-12 error margin.
; - the top 128 bits of ymms registers are zero.
; - bits [127:32] are correctly copied from the first argument to vrcpss.
_start:
vmovapd ymm0, [rel arg1]
vmovapd ymm1, [rel arg2]

; Register only
vrcpss xmm2, xmm1, xmm0

; Memory operand
vrcpss xmm3, xmm0, [rel arg2]

; Check precision
vpextrd [rel result], xmm2, 0
check_relerr rel eresult1, rel result, rel tolerance
and r9, rax

vpextrd [rel result], xmm3, 0
check_relerr rel eresult2, rel result, rel tolerance
mov r9, rax

; Insert 0s in the bottom 32bits
xor rax, rax
vpinsrd xmm2, xmm2, eax, 0
vpinsrd xmm3, xmm3, eax, 0
hlt

align 4096
result: times 2 dq 0

align 32
arg1:
dq 0x3F8000003F800000 ; 1.0
dq 0x3F8000003F800000
dq 0x3F8000003F800000
dq 0x3F8000003F800000

arg2:
dq 0x4080000040800000 ; 4.0
dq 0x4080000040800000
dq 0x4080000040800000
dq 0x4080000040800000

eresult1:
dd 0x3F800000 ; 1.0

eresult2:
dd 0x3e800000 ; 0.25

tolerance:
dd 0x39c00000

define_check_data_constants
