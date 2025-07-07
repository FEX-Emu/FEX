%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

; Test (-∞) - (-∞) = Invalid Operation (should set bit 0 of status word)
fld1
fchs
fldz
fdiv ; st0 = -∞

; Duplicate -infinity on stack
fld st0

; Subtract (-∞) - (-∞) - this should be invalid
fsub

fstsw ax
and rax, 1

hlt