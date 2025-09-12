%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "MemoryRegions": {
    "0x100000000": "4096",
    "0x100002000": "4096"
  }
}
%endif

mov r15, 0x100001000
mov r14, 0x100002000
mov rax, 0

%include "modrm_oob_macros.mac"

; X87

; These macros using the w* versions are actually reads.
%macro x87_f48_op 1
fldz
w3_size %1, 4, dword
w3_size %1, 8, qword
ffreep
%endmacro

%macro x87_i24_op 1
fldz
w3_size %1, 2, word
w3_size %1, 4, dword
ffreep
%endmacro

x87_f48_op fadd
x87_f48_op fmul
x87_f48_op fcom

; fcomp is special
fldz
w3_size fcomp, 4, dword
fldz
w3_size fcomp, 8, qword

x87_f48_op fsub
x87_f48_op fsubr
x87_f48_op fdiv
x87_f48_op fdivr

; fld is special
w3_size fld, 4, dword
ffreep
w3_size fld, 8, qword
ffreep
w3_size fld, 10, tword
ffreep

; fst is special
fldz
w3_size fst, 4, dword
w3_size fst, 8, qword

; fstp is special
fldz
w3_size fstp, 4, dword
fldz
w3_size fstp, 8, qword
fldz
w3_size fstp, 10, tword

w2 fnstenv, 28
w2 fldenv, 28

w2 o16 fnstenv, 14
w2 o16 fldenv, 14

w2 fnstcw, 2
w2 fldcw, 2

x87_i24_op fiadd
x87_i24_op fimul
x87_i24_op ficom

; ficomp is special
fldz
w3_size ficomp, 2, word
fldz
w3_size ficomp, 4, dword

x87_i24_op fisub
x87_i24_op fisubr

x87_i24_op fidiv
x87_i24_op fidivr

; fild is special
w3_size fild, 2, word
ffreep
w3_size fild, 4, dword
ffreep
w3_size fild, 8, qword
ffreep

; fist is special
fldz
w3_size fist, 2, word
w3_size fist, 4, dword

; fistp is special
fldz
w3_size fistp, 2, word
fldz
w3_size fistp, 4, dword
fldz
w3_size fistp, 8, qword

; fisttp is special
fldz
w3_size fisttp, 2, word
fldz
w3_size fisttp, 4, dword
fldz
w3_size fisttp, 8, qword

w2 fnsave, 108
w2 frstor, 108

w2 fnstsw, 2

w2 o16 fnsave, 94
w2 o16 frstor, 94

w3_size fbld, 10, tword
w3_size fbstp, 10, tword

; Done
mov rax, 1
hlt
