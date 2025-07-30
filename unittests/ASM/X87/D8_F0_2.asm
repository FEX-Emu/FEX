%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0"
  }
}
%endif

; Tests that a division by zero does not set the IE flag
finit
fldz
fld1
fdiv st0, st1

fnstsw ax
and rax, 1
hlt
