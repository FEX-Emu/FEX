%ifdef CONFIG
{
  "RegData": {
    "RBX": "1",
    "MM3": "0xff8000007f800000"
  },
  "HostFeatures": ["3DNOW", "EMMI"]
}
%endif

%include "checkprecision.mac"

section .text
global _start

_start:
pfrcpv mm0, [rel data1]
pfrcpv mm1, [rel data2]
pfrcpv mm2, [rel data3]
pfrcpv mm3, [rel data4]

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

align 4096
result11: dd 0
result12: dd 0
result21: dd 0
result22: dd 0
result31: dd 0
result32: dd 0

align 8
data1:
dd -1.0
dd 1.0

data2:
dd -128.0
dd 128.0

data3:
dd 1.0
dd -1.0

data4:
dd 0.0
dd -0.0

eresult11:
dd -1.0
eresult12:
dd 1.0
eresult21:
dd 0xbc000000 ; -1/128
eresult22:
dd 0x3c000000 ; 1/128
eresult31:
dd 1.0
eresult32:
dd -1.0

tolerance:
dd 0x38800000 ; 2^-14 - 14bit accuracy

define_check_data_constants
