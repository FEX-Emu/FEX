%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Mode": "32BIT"
}
%endif

; Test ∞ - ∞ using FSUBR = Invalid Operation (should set bit 0 of status word) - 32bit mode
fld1
fldz
fdiv ; st0 = +∞

; duplicate +infinity
fld st0

; Reverse subtract ∞ - ∞ using FSUBR - this should be invalid
fsubr

fstsw ax
and eax, 1

hlt