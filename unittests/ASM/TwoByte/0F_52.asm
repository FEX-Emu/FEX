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

;; This test checks that the rsqrtps returns results within the 1.5*2^-12 relative error
;; margin and that the results are packed as a vector of 4 32bits. Because we pass in
;; the same argument accross the vector we expect the same result accross the vector
;; and we check that with macro same_pdwords

movapd xmm0, [rel arg1]
movapd xmm1, [rel arg2]
movapd xmm2, [rel arg3]
movapd xmm3, [rel arg4]
movapd xmm4, [rel arg5]
movapd xmm5, [rel arg5]
movapd xmm6, [rel arg5]
movapd xmm7, [rel arg5]

rsqrtps xmm0, xmm0
rsqrtps xmm1, xmm1
rsqrtps xmm2, xmm2
rsqrtps xmm3, xmm3

rsqrtps xmm4, [rel arg1]
rsqrtps xmm5, [rel arg2]
rsqrtps xmm6, [rel arg3]
rsqrtps xmm7, [rel arg4]

same_pdwords xmm0
mov r8, rax
same_pdwords xmm1
and r8, rax
same_pdwords xmm2
and r8, rax
same_pdwords xmm3
and r8, rax
same_pdwords xmm4
and r8, rax
same_pdwords xmm5
and r8, rax
same_pdwords xmm6
and r8, rax
same_pdwords xmm7
and r8, rax

pextrd [rel result1], xmm0, 0
pinsrd xmm0, esi, 0
check_relerr rel eresult1, rel result1, rel tolerance
mov r9, rax

pextrd [rel result2], xmm1, 0
pinsrd xmm1, esi, 0
check_relerr rel eresult2, rel result2, rel tolerance
and r9, rax

pextrd [rel result3], xmm2, 0
pinsrd xmm2, esi, 0
check_relerr rel eresult3, rel result3, rel tolerance
and r9, rax

pextrd [rel result4], xmm3, 0
pinsrd xmm3, esi, 0
check_relerr rel eresult4, rel result4, rel tolerance
and r9, rax

hlt

section .bss
result1: resd 1
result2: resd 1
result3: resd 1
result4: resd 1

section .data
align 64

arg1: 
dq 0x3f8000003f800000 ; 1.0
dq 0x3f8000003f800000

arg2: 
dq 0x4080000040800000 ; 4.0
dq 0x4080000040800000

arg3:
dq 0x4110000041100000 ; 9.0
dq 0x4110000041100000

arg4: 
dq 0x4180000041800000 ; 16.0
dq 0x4180000041800000

arg5:
dq 0x41c8000041c80000 ; 25.0
dq 0x41c8000041c80000

align 32
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
