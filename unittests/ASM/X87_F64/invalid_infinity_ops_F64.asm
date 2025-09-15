%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; Test ∞ + (-∞) = Invalid Operation (should set bit 0 of status word) - reduced precision
; Create +infinity by dividing 1.0 by 0.0
fld1
fldz
fdiv

; Create -infinity by dividing -1.0 by 0.0
fld1
fchs
fldz
fdiv

; Add +∞ + (-∞) - this should be invalid
fadd

fstsw ax
and rax, 1

hlt
