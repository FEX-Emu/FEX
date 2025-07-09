%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Mode": "32BIT"
}
%endif

; Test ∞ - ∞ = Invalid Operation (should set bit 0 of status word)
; Create +infinity by dividing 1.0 by 0.0
fld1
fldz
fdiv

; Create another +infinity
fld1
fldz
fdiv

; Subtract: +∞ - +∞ -> Invalid Operation
fsub

fstsw ax
and eax, 1

hlt
