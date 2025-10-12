%ifdef CONFIG
{
  "RegData": {
    "R8": "1",
    "R9": "1"
  },
  "HostFeatures": ["SSE4.1"]
}
%endif

%include "checkprecision.mac"

; clobbers xmm15
; returns the comparison result in rax
%macro same_pdwords 1 ; receives the xmms register
    movd eax, %1
    movd xmm15, eax
    pshufd xmm15, xmm15, 0 ; has the lower 32bits of %1 accross all lanes
    pcmpeqd xmm15, %1 ; has equalty mask on all lanes
    movmskps eax, xmm15 ; gets sign bit of each lane into eax
    cmp eax, 0b1111
    sete al
    movzx rax, al
%endmacro

section .text
global _start

_start:

movapd xmm0, [rel arg1]

rcpps xmm0, xmm0
rcpps xmm1, [rel arg2]

same_pdwords xmm0
mov r8, rax
same_pdwords xmm1
and r8, rax

pextrd [rel result1], xmm0, 0
pinsrd xmm0, esi, 0
check_relerr rel eresult1, rel result1, rel tolerance
mov r9, rax

pextrd [rel result2], xmm1, 0
pinsrd xmm1, esi, 0
check_relerr rel eresult2, rel result2, rel tolerance
and r9, rax

hlt

align 4096
result1: dd 0
result2: dd 0

align 64

arg1:
dq 0x3f8000003f800000 ; 1.0
dq 0x3f8000003f800000

arg2:
dq 0x4080000040800000 ; 4.0
dq 0x4080000040800000


align 32
eresult1:
dd 0x3f800000 ; 1.0
eresult2:
dd 0x3e800000 ; 0.25

tolerance:
dd 0x39c00000

define_check_data_constants
