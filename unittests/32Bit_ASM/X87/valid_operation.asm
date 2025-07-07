%ifdef CONFIG
{
  "RegData": {
    "RAX": "0"
  },
  "Mode": "32BIT"
}
%endif

; Test a valid operation that should NOT set Invalid Operation bit
fld1
fld1
fadd

fstsw ax
and eax, 1

hlt
