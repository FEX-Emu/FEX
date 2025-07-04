%ifdef CONFIG
{
  "RegData": {
    "RAX": "0"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; Test valid operation (should NOT set bit 0 of status word) - reduced precision
fld1
fld1
fadd

fstsw ax
and rax, 1

hlt
