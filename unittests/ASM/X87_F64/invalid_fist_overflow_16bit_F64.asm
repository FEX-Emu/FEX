%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

section .rodata
.thirty: dq 30

section .bss
.dummy: resw 1

section .text
global _start

_start:

; Test FIST with 16-bit overflow = Invalid Operation (should set bit 0 of status word) - reduced precision
; Create a large number that will overflow int16

; Load 2^20 (larger than int16 range: max int16 = 32767, 2^20 = 1048576)
finit
fild dword [rel .thirty]
fld1
fscale

; Try to convert to int16 - this should overflow and be invalid
fistp word [rel .dummy]

fstsw ax
and rax, 1

hlt
