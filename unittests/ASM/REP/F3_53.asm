%ifdef CONFIG
{
  "RegData": {
    "RAX": "1",
    "RBX": "1",
    "XMM0":  ["0x3f80000000000000", "0x3f8000003f800000"],
    "XMM1":  ["0x4080000000000000", "0x4080000040800000"],
    "XMM2":  ["0xdeadbeef7f800000", "0xbadc0ffebadc0ffe"]
  },
  "HostFeatures": ["SSE4.1"]
}
%endif

; Check precision of the results
; while ensuring we didn't destroy the rest of the register.
%include "checkprecision.mac"

section .text
global _start

_start:
movapd xmm0, [rel arg1]
movapd xmm1, [rel arg2]
movapd xmm2, [rel arg3]

rcpss xmm0, xmm0
rcpss xmm1, [rel arg2]
rcpss xmm2, xmm2

xor esi, esi

pextrd [rel result1], xmm0, 0
pinsrd xmm0, esi, 0
check_relerr rel eresult1, rel result1, rel tolerance
mov rbx, rax

pextrd [rel result2], xmm1, 0
pinsrd xmm1, esi, 0
check_relerr rel eresult2, rel result2, rel tolerance

hlt


section .bss
align 32
result1 resd 1
result2 resd 1

section .data
align 16

arg1: 
dq 0x3f8000003f800000 ; 1.0
dq 0x3f8000003f800000

arg2:
dq 0x4080000040800000 ; 4.0
dq 0x4080000040800000

arg3:
dq 0xdeadbeef00000000 ; 0.0
dq 0xbadc0ffebadc0ffe

eresult1: 
dd 0x3f800000 ; 1.0

eresult2:
dd 0x3e800000 ; 0.5

tolerance:
dd 0x39c00000

define_check_data_constants
