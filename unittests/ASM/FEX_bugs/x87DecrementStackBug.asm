%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x4000000000000000", "0"]
  }
}
%endif

; FEX-Emu contains a bug. It's the buggiest bug that ever bugged. Something about conflicting results between fxch and fincstp.
fld tword [rel .data1]
fld tword [rel .data2]
fld tword [rel .data3]
fld tword [rel .data4]
fld tword [rel .data5]
fld tword [rel .data6]
fld tword [rel .data7]
fld tword [rel .data8]

jmp .test

.test:
fxch st0, st1
fdecstp

jmp .end
.end:

fstp qword [rel .data_result]
movups xmm0, [rel .data_result]

hlt

.data1:
dt 2.0
dq 0

.data2:
dt 4.0
dq 0

.data3:
dt 8.0
dq 0

.data4:
dt 16.0
dq 0

.data5:
dt 32.0
dq 0

.data6:
dt 64.0
dq 0

.data7:
dt 128.0
dq 0

.data8:
dt 256.0
dq 0

.data_result:
dq 0
dq 0
