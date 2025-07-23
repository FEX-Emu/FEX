%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Mode": "32BIT"
}
%endif

; Test ∞ - ∞ = Invalid Operation (should set bit 0 of status word)
fld1
fldz
fdiv ; st0 = +∞

; Duplicate +infinity on stack
fld st0

; Subtract: +∞ - +∞ -> Invalid Operation
fsub

fstsw ax
and eax, 1

hlt
