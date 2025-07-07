%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Mode": "32BIT"
}
%endif

; Test ∞ × 0 = Invalid Operation (should set bit 0 of status word)
fld1
fldz
fdiv ; st0 = +∞

; Load 0.0 and multiply with infinity
fldz
fmul

fstsw ax
and eax, 1

hlt
