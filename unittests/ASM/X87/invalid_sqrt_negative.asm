%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

; Test sqrt(-1.0) = Invalid Operation (should set bit 0 of status word)
fld1
fchs
fsqrt

fstsw ax
and rax, 1

hlt
