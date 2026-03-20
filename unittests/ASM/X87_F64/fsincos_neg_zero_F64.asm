%ifdef CONFIG
{
  "RegData": {
    "RAX":  "1",
    "RBX":  "0x8000000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

%include "checkprecision.mac"

mov rcx, 0xe0000000

lea rdx, [rel data]
fld qword [rdx]

fsincos

; ST(0) = cos(-0.0), ST(1) = sin(-0.0) = -0.0
fstp qword [rcx]
check_relerr_d rel expected_cos, rcx, rel tolerance

fstp qword [rcx]
mov rbx, [rcx]

hlt

align 8
data:
  dq 0x8000000000000000 ; -0.0
expected_cos:
  dq 0x3ff0000000000000 ; 1.0
tolerance:
  dq 0x3cb0000000000000 ; 2^-52, ~1 ULP relative error

define_check_data_constants
