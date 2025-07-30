%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "RBX": "1"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; We do one an invalid operation, then a valid one.
; The bit should be set correctly after the invalid operation,
; and cleared by the valid operation.
fldz
fldz
fdiv ; division by zero.

fstsw ax
mov bx, ax
and bx, 1

fld1
fld1
fadd ; valid operation

fstsw ax
and ax, 1

hlt
