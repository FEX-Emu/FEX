%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

; Test ∞ - ∞ using FSUB
fld1
fldz
fdiv ; st0 = +∞

; duplicate +infinity
fld st0

; Reverse subtract ∞ - ∞ using FSUBR - this should be invalid
fsubr

fstsw ax
and rax, 1

hlt