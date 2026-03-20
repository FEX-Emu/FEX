%ifdef CONFIG
{
  "RegData": {
    "RAX":  "1"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

%include "checkprecision.mac"

mov rbx, 0xe0000000

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

fsin

fst qword [rbx]

check_relerr_d rel expected, rbx, rel tolerance
hlt

align 8
data:
  dt 1.0
  dq 0
expected:
  dq 0x3feaed548f090cee ; sin(1.0)
tolerance:
  dq 0x3cb0000000000000 ; 2^-52, ~1 ULP relative error

define_check_data_constants
