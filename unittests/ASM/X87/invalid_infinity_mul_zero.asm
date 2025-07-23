%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

; Test ∞ × 0 = Invalid Operation (should set bit 0 of status word)
fld1
fldz
fdiv ; st0 = +∞

; Load zero for multiplication
fldz

; Multiply infinity by zero - this should be invalid
fmul

fstsw ax
and rax, 1

hlt
