%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Mode": "32BIT"
}
%endif

; Test (-∞) - (-∞) = Invalid Operation (should set bit 0 of status word) - 32bit mode
; Create -infinity by dividing -1.0 by 0.0
fld1
fchs
fldz
fdiv

; duplicate -infinity on stack
fld st0

; Subtract (-∞) - (-∞) - this should be invalid
fsub

fstsw ax
and eax, 1

hlt