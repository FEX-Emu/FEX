%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

; Test ∞ - ∞ = Invalid Operation (should set bit 0 of status word)
fld1
fldz
fdiv ; st0 = +∞

; Duplicate infinity on stack
fld st0

; Create -infinity by changing sign
fchs

; Now compute +∞ + (-∞) which should be invalid
fadd

fstsw ax
and rax, 1

hlt
