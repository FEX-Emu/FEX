%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; Test ∞ - ∞ using FSUBR = Invalid Operation (should set bit 0 of status word) - reduced precision
; Create +infinity by dividing 1.0 by 0.0
fld1
fldz
fdiv

; Create +infinity by dividing 1.0 by 0.0 
fld1
fldz
fdiv

; Reverse subtract ∞ - ∞ using FSUBR - this should be invalid
fsubr

fstsw ax
and rax, 1

hlt