%ifdef CONFIG
{
  "RegData": {
    "RAX": "0"
  }
}
%endif

; Test valid operation should NOT set Invalid Operation bit
; Clear any existing exception flags first
finit

; Perform valid operations
fld1
fld1
fadd

fld1
fdiv

fld1
fsqrt

fstsw ax
and rax, 1

hlt
