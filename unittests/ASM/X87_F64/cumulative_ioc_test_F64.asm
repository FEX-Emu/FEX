%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "RBX": "1"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; We perform one an invalid operation.
fldz
fldz
fdiv ; division by zero.

fstsw ax
mov bx, ax
and bx, 1 ; IE flag set

fld1
fld1
fadd 

fstsw ax
and ax, 1 ; IE flag still set because of the previous invalid operation
and bx, ax

fnclex
fld1
fld1
fadd 

fstsw ax
and ax, 1 ; IE flag unset because fnclex cleared the status word

hlt
