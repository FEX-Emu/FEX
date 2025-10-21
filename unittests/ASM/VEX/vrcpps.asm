%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "R8": "1",
    "R9": "1"
  }
}
%endif


section .text
global _start

%include "checkprecision.mac"

; clobbers ymm15
; returns the comparison result in rax
%macro same_pdwords 1 ; receives the ymms register
    vextractf128 xmm15, %1, 0
    vmovd eax, xmm15
    vmovd xmm15, eax
    vbroadcastss ymm15, xmm15  ; broadcast lower 32bits across all 8 lanes
    vpcmpeqd ymm15, ymm15, %1  ; equality mask on all lanes
    vmovmskps eax, ymm15       ; gets sign bit of each lane into eax
    cmp eax, 0b11111111        ; check all 8 lanes
    sete al
    movzx rax, al
%endmacro

; clobbers xmm15
; returns the comparison result in rax
%macro same_pdwords_x 1 ; receives the xmms register
    movd eax, %1
    movd xmm15, eax
    pshufd xmm15, xmm15, 0 ; has the lower 32bits of %1 accross all lanes 
    pcmpeqd xmm15, %1 ; has equalty mask on all lanes
    movmskps eax, xmm15 ; gets sign bit of each lane into eax
    cmp eax, 0b1111
    sete al
    movzx rax, al
%endmacro

_start:
vmovapd ymm0, [rel arg1]
vmovapd ymm1, [rel arg2]

; Register only
vrcpps ymm2, ymm0
vrcpps xmm3, xmm0

; Memory operand
vrcpps ymm4, [rel arg2]
vrcpps xmm5, [rel arg2]

; Check that each register is properly filled
same_pdwords ymm2
mov r8, rax

same_pdwords_x xmm3
and r8, rax

same_pdwords ymm4
and r8, rax

same_pdwords_x xmm5
and r8, rax

; Result checks
vpextrd [rel result], xmm2, 0
check_relerr rel eresult1, rel result, rel tolerance
mov r9, rax

vpextrd [rel result], xmm3, 0
check_relerr rel eresult1, rel result, rel tolerance
and r9, rax

vpextrd [rel result], xmm4, 0
check_relerr rel eresult2, rel result, rel tolerance
and r9, rax

vpextrd [rel result], xmm5, 0
check_relerr rel eresult2, rel result, rel tolerance
and r9, rax
hlt

align 4096
result: times 4 dq 0

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
