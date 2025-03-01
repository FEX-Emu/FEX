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
vmovapd ymm2, [rel arg3]
vmovapd ymm3, [rel arg4]
vmovapd ymm4, [rel arg5]
vmovapd ymm5, [rel arg5]
vmovapd ymm6, [rel arg5]
vmovapd ymm7, [rel arg5]

; Register only
vrsqrtps ymm0, ymm0
vrsqrtps ymm1, ymm1
vrsqrtps xmm2, xmm2
vrsqrtps xmm3, xmm3

; Memory operand
vrsqrtps ymm4, [rel arg1]
vrsqrtps ymm5, [rel arg2]
vrsqrtps xmm6, [rel arg3]
vrsqrtps xmm7, [rel arg4]

; Check that each register is properly filled
same_pdwords ymm0
mov r8, rax

same_pdwords ymm1
and r8, rax

same_pdwords_x xmm2
and r8, rax

same_pdwords_x xmm3
and r8, rax

; Result checks
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
hlt

section .bss
align 32
result1: resq 4
result2: resq 4
result3: resq 4
result4: resq 4

section .data
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

arg3:
dq 0x4110000041100000 ; 9.0
dq 0x4110000041100000
dq 0x4110000041100000
dq 0x4110000041100000

arg4:
dq 0x4180000041800000 ; 16.0
dq 0x4180000041800000
dq 0x4180000041800000
dq 0x4180000041800000

arg5:
dq 0x41C8000041C80000 ; 25.0
dq 0x41C8000041C80000
dq 0x41C8000041C80000
dq 0x41C8000041C80000

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