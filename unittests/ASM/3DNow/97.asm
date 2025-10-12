%ifdef CONFIG
{
  "RegData": {
    "RAX":  "1",
    "RBX":  "1",
    "RCX":  "1",
    "RDX":  "1",
    "MM4":  "0x7f8000007f800000",
    "MM5":  "0xff800000ff800000"
  },
  "HostFeatures": ["3DNOW"]
}
%endif

%include "checkprecision.mac"

section .text
global _start

; From the isa manual, one thing to consider is that
; "Negative operands are treated as positive operands for purposes of
; reciprocal square-root computation, with the sign of the result the
; same as the sign of the source operand."

_start:
pfrsqrt mm0, [rel data1]
movd [rel result1], mm0
check_relerr rel eresult1, rel result1, rel tolerance
movzx rdx, al

pfrsqrt mm1, [rel data2]
movd [rel result2], mm1
check_relerr rel eresult2, rel result2, rel tolerance
movzx rcx, al

pfrsqrt mm2, [rel data3]
movd [rel result3], mm2
check_relerr rel eresult3, rel result3, rel tolerance
movzx rbx, al

pfrsqrt mm3, [rel data4] ; pfrsqrt(-1.0) == -1.0
movd [rel result4], mm3
check_relerr rel eresult4, rel result4, rel tolerance
movzx rax, al

; Expecting exact results
pfrsqrt mm4, [rel data5] ; pfrsqrt(0.0) == inf
pfrsqrt mm5, [rel data6] ; pfrsqrt(-0.0) == -inf
hlt

align 4096
result1: times 32 db 0
result2: times 32 db 0
result3: times 32 db 0
result4: times 32 db 0

align 32
data1:
dd 1.0
dd 16.0

eresult1: ; expected
dd 1.0

data2:
dd 4.0
dd 25.0

eresult2: ; expected
dd 0.5

data3:
dd 9.0
dd 1.0

eresult3: ; expected
dd 0x3eaaaaab ; 1/3

data4:
dd -1.0
dd -16.0

eresult4: ; expected
dd -1.0

data5:
dd 0.0
dd -9.0

data6:
dd -0.0
dd -9.0

tolerance:
dd 0x38000000 ; 2^-15 - accurate to 15bits

define_check_data_constants
