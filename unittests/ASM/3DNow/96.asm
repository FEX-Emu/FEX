%ifdef CONFIG
{
  "RegData": {
    "RCX": "1",
    "RDX": "1",
    "MM0":  "0x7f8000007f800000",
    "MM1":  "0xff800000ff800000"
  },
  "HostFeatures": ["3DNOW"]
}
%endif

%include "checkprecision.mac"

; For each operation:
; * We check precision (except when checking exact values), for 0.0 and -0.0.
; * Check that top and bottom of register has the same value.

section .text
global _start

_start:
pfrcp mm0, [rel data1]

; Precision
movd [rel result], mm0
check_relerr rel eresult1, rel result, rel tolerance
movzx rdx, al
; Duplicate top/bottom
movq mm1, mm0
psrlq mm1, 32
pcmpeqd mm0, mm1
movd eax, mm0
and al, 1
movzx rcx, al

pfrcp mm0, [rel data2]

; Precision
movd [rel result], mm0
check_relerr rel eresult2, rel result, rel tolerance
and rdx, rax
; Duplicate top/bottom
movq mm1, mm0
psrlq mm1, 32
pcmpeqd mm0, mm1
movd eax, mm0
and al, 1
and rcx, rax

pfrcp mm0, [rel data3]

; Precision
movd [rel result], mm0
check_relerr rel eresult3, rel result, rel tolerance
and rdx, rax
; Duplicate top/bottom
movq mm1, mm0
psrlq mm1, 32
pcmpeqd mm0, mm1
movd eax, mm0
and al, 1
and rcx, rax

; ; Expecting exact results for +inf and -inf
pfrcp mm0, [rel data4]
pfrcp mm1, [rel data5]

hlt

align 4096
result: dd 0

align 8
data1:
dd -1.0
dd 1.0

eresult1:
dd -1.0

data2:
dd -128.0
dd 128.0

eresult2:
dd 0xbc000000

data3:
dd 1.0
dd -1.0

eresult3:
dd 1.0

data4:
dd 0.0
dd 1.0

data5:
dd -0.0
dd 1.0

tolerance:
dd 0x38800000 ; 2^-14 - 14bit accuracy

define_check_data_constants
