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

; Duplicate infinity on stack
fld st0

; Create -infinity by changing sign
fchs

; Now compute +∞ + (-∞) which should be invalid
fadd

fstsw ax
and rax, 1

hlt
