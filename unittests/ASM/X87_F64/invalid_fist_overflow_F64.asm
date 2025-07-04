%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

section .rodata
.fifty: dq 50

section .bss
.dummy: resb 4

section .text
global _start

_start:

; Test FIST with overflow = Invalid Operation (should set bit 0 of status word) - reduced precision
; Create a very large number that will overflow int32

; Load 2^32 (larger than int32 range)
finit
fild dword [rel .fifty]
fld1
fscale

; Try to convert to int32 - this should overflow and be invalid
fistp dword [rel .dummy]

fstsw ax
and rax, 1

hlt
