%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; Test ∞ × 0 = Invalid Operation (should set bit 0 of status word) - reduced precision
; Create +infinity by dividing 1.0 by 0.0
fld1
fldz
fdiv

; Load zero for multiplication
fldz

; Multiply infinity by zero - this should be invalid
fmul

fstsw ax
and rax, 1

hlt
