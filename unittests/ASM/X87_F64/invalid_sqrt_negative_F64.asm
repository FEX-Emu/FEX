%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; Test sqrt(-1) = Invalid Operation (should set bit 0 of status word) - reduced precision
fld1
fchs
fsqrt

fstsw ax
and rax, 1

hlt
