%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x4010000000000000", "0"]
  }
}
%endif

; FEX-Emu contains a bug. It's the buggiest bug that ever bugged. Something about conflicting results between fxch and fincstp.
fld tword [rel .data1]
fld tword [rel .data2]

; ST(0) contains 4.0
; ST(1) contains 2.0

jmp .test

.test:
fxch st0, st1
; ST(0) now contains 2.0
; ST(1) now contains 4.0

fincstp
; ST(0) now contains 4.0
; ST(7) now contains 2.0

jmp .end
.end:

fstp qword [rel .data_result]
movups xmm0, [rel .data_result]

hlt

; This or zero are incorrect results
.data1:
dt 2.0
dq 0

; Correct result
.data2:
dt 4.0
dq 0

.data_result:
dq 0
dq 0
