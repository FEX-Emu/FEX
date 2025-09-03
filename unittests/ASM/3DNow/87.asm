%ifdef CONFIG
{
  "RegData": {
    "RBX": "1"
  },
  "HostFeatures": ["3DNOW", "EMMI"]
}
%endif

%include "checkprecision.mac"

pfrsqrtv mm0, [rel data1]
pfrsqrtv mm1, [rel data2]
pfrsqrtv mm2, [rel data3]

; All calculated
; Now we extract all the values into memory to call check_relerr.
movd edx, mm0
mov [rel result11], edx

psrlq mm0, 32
movd edx, mm0
mov [rel result12], edx

movd edx, mm1
mov [rel result21], edx

psrlq mm1, 32
movd edx, mm1
mov [rel result22], edx

movd edx, mm2
mov [rel result31], edx

psrlq mm2, 32
movd edx, mm2
mov [rel result32], edx

check_relerr rel eresult11, rel result11, rel tolerance
mov ebx, eax
check_relerr rel eresult12, rel result12, rel tolerance
and ebx, eax
check_relerr rel eresult21, rel result21, rel tolerance
and ebx, eax
check_relerr rel eresult22, rel result22, rel tolerance
and ebx, eax
check_relerr rel eresult31, rel result31, rel tolerance
and ebx, eax
check_relerr rel eresult32, rel result32, rel tolerance
and ebx, eax

hlt

section .bss
align 32
result11: resd 1
result12: resd 1
result21: resd 1
result22: resd 1
result31: resd 1
result32: resd 1

section .data
align 32
data1:
dd 1.0
dd 16.0

data2:
dd 4.0
dd 25.0

data3:
dd 9.0
dd 1.0

eresult11:
dd 0x3f800000 ; 1.0
eresult12:
dd 0x3e800000 ; 1/4 = 0.25
eresult21:
dd 0x3f000000 ; 1/2 = 0.5
eresult22:
dd 0x3e4ccccd ; 1/5 = 0.2
eresult31:
dd 0x3eaaaaab ; 1/3 = 0.(3)
eresult32:
dd 0x3f800000 ; 1.0

tolerance:
dd 0x38000000 ; 2^-15 - accurate to 15bits

define_check_data_constants
