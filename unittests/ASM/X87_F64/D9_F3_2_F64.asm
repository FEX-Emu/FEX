%ifdef CONFIG
{
  "RegData": {
    "RAX":  "1"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

%include "checkprecision.mac"

; Test fpatan (atan2) with multiple quadrant cases in double precision.

mov rsi, 0xe0000000

; atan2(1.0, 1.0) = pi/4
lea rdx, [rel one_pos]
fld tword [rdx]
fld tword [rdx]
fpatan
fstp qword [rsi]
check_relerr_d rel expected_pi4, rsi, rel tolerance
mov r8, rax

; atan2(1.0, -1.0) = 3*pi/4
lea rdx, [rel one_pos]
fld tword [rdx]
lea rdx, [rel one_neg]
fld tword [rdx]
fpatan
fstp qword [rsi]
check_relerr_d rel expected_3pi4, rsi, rel tolerance
and r8, rax

; atan2(-1.0, 1.0) = -pi/4
lea rdx, [rel one_neg]
fld tword [rdx]
lea rdx, [rel one_pos]
fld tword [rdx]
fpatan
fstp qword [rsi]
check_relerr_d rel expected_neg_pi4, rsi, rel tolerance
and rax, r8

hlt

align 8
one_pos:
  dt 1.0
  dq 0
one_neg:
  dt -1.0
  dq 0
expected_pi4:
  dq 0x3fe921fb54442d18 ; pi/4
expected_3pi4:
  dq 0x4002d97c7f3321d2 ; 3*pi/4
expected_neg_pi4:
  dq 0xbfe921fb54442d18 ; -pi/4
tolerance:
  dq 0x3cb0000000000000 ; 2^-52, ~1 ULP relative error

define_check_data_constants
