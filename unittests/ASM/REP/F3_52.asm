%ifdef CONFIG
{
  "RegData": {
    "RAX": "1",
    "RBX": "1",
    "RCX": "1",
    "RDX": "1",
    "XMM0":  ["0x4142434400000000", "0x5152535455565758"],
    "XMM1":  ["0x4142434400000000", "0x5152535455565758"],
    "XMM2":  ["0x4142434400000000", "0x5152535455565758"],
    "XMM3":  ["0x4142434400000000", "0x5152535455565758"],
    "XMM4":  ["0x4142434400000000", "0x5152535455565758"],
    "XMM5":  ["0x4142434400000000", "0x5152535455565758"],
    "XMM6":  ["0x4142434400000000", "0x5152535455565758"],
    "XMM7":  ["0x4142434400000000", "0x5152535455565758"]
  },
  "HostFeatures": ["SSE4.1"]
}
%endif


section .text
global _start

_start:
movapd xmm0, [rel arg1]
movapd xmm1, [rel arg2]
movapd xmm2, [rel arg3]
movapd xmm3, [rel arg4]
movapd xmm4, [rel arg5]
movapd xmm5, [rel arg5]
movapd xmm6, [rel arg5]
movapd xmm7, [rel arg5]

rsqrtss xmm0, xmm0
rsqrtss xmm1, xmm1
rsqrtss xmm2, xmm2
rsqrtss xmm3, xmm3

rsqrtss xmm4, [rel arg1]
rsqrtss xmm5, [rel arg2]
rsqrtss xmm6, [rel arg3]
rsqrtss xmm7, [rel arg4]


; Check precision of the results
; while ensuring we didn't destroy the rest of the register.
%include "checkprecision.mac"

;; We will be storing the low 32bits to memory, then zeroing them.
;; We'll then check precision using checkprecision.mac.
; Zero rsi:
xor esi, esi

pextrd [rel result1], xmm0, 0
pinsrd xmm0, esi, 0
check_relerr rel eresult1, rel result1, rel tolerance
mov rbx, rax

pextrd [rel result2], xmm1, 0
pinsrd xmm1, esi, 0
check_relerr rel eresult2, rel result2, rel tolerance
mov rcx, rax

pextrd [rel result3], xmm2, 0
pinsrd xmm2, esi, 0
check_relerr rel eresult3, rel result3, rel tolerance
mov rdx, rax

pextrd [rel result4], xmm3, 0
pinsrd xmm3, esi, 0
check_relerr rel eresult4, rel result4, rel tolerance

; no need to test the other results which are the same, 
; we can just zero them.
pinsrd xmm4, esi, 0
pinsrd xmm5, esi, 0
pinsrd xmm6, esi, 0
pinsrd xmm7, esi, 0
hlt

section .bss
align 32
result1 resd 1
result2 resd 1
result3 resd 1
result4 resd 1

section .data
align 16

arg1: 
dq 0x414243443f800000 ; 1.0
dq 0x5152535455565758

arg2:
dq 0x4142434440800000 ; 4.0
dq 0x5152535455565758

arg3:
dq 0x4142434441100000 ; 9.0
dq 0x5152535455565758

arg4:
dq 0x4142434441800000 ; 16.0
dq 0x5152535455565758

arg5:
dq 0x4142434441c80000 ; 25.0
dq 0x5152535455565758

eresult1: 
dd 0x3f800000 ; 1.0

eresult2:
dd 0x3f000000 ; 0.5

eresult3: 
dd 0x3eaaaaab ; 1/3 = 0.(3)

eresult4:
dd 0x3e800000 ; 0.25

tolerance:
dd 0x39c00000

define_check_data_constants
