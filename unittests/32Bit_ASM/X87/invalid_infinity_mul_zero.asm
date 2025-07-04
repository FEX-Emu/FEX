%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Mode": "32BIT"
}
%endif

; Test ∞ × 0 = Invalid Operation (should set bit 0 of status word)
; Create +infinity by dividing 1.0 by 0.0
fld1
fldz
fdiv

; Load 0.0 and multiply with infinity
fldz
fmul

fstsw ax
and eax, 1

hlt
