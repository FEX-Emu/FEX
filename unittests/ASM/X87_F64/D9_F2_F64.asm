%ifdef CONFIG
{
  "RegData": {
    "RAX":  "1",
    "RBX":  "0x3ff0000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

%include "checkprecision.mac"

mov rcx, 0xe0000000

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

fptan

; ST(0) = 1.0, ST(1) = tan(1.0)
fstp qword [rcx]
mov rbx, [rcx]

fstp qword [rcx]
check_relerr_d rel expected_tan, rcx, rel tolerance

hlt

align 8
data:
  dt 1.0
  dq 0
expected_tan:
  dq 0x3ff8eb245cbee3a5 ; tan(1.0)
tolerance:
  dq 0x3cb0000000000000 ; 2^-52, ~1 ULP relative error

define_check_data_constants
