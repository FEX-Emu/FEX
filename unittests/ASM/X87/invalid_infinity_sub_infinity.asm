%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

; Test ∞ - ∞ = Invalid Operation (should set bit 0 of status word)
; Create +infinity by dividing 1.0 by 0.0
fld1
fldz
fdiv

; Create +infinity by dividing 1.0 by 0.0 
fld1
fldz
fdiv

; Subtract +∞ - ∞ - this should be invalid
fsub

fstsw ax
and rax, 1

hlt