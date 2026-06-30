%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

%include "checkprecision.mac"

mov rsi, 0xe0000000

; y=1, x = 0.25
fld1
lea rdx, [rel x_quarter]
fld qword [rdx]
fyl2xp1
fstp qword [rsi]
check_relerr_d rel expected_quarter, rsi, rel tolerance

hlt

align 8
x_quarter:
  dq 0x3FD0000000000000
expected_quarter:
  dq 0x3FD49A784BCD1B87 ; matches the LUT-based F64 FYL2X path
tolerance:
  dq 0x3CD0000000000000 ; 2^-50

define_check_data_constants
