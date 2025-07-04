%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

section .rodata
.seventyfive: dq 75

section .bss
.dummy: resq 1

section .text
global _start

_start:

; Test FIST with 64-bit overflow = Invalid Operation (should set bit 0 of status word)
; Create a very large number that will overflow int64

; Load 2^75 (larger than int64 range)
finit
fild dword [rel .seventyfive]
fld1
fscale

; Try to convert to int64 - this should overflow and be invalid
fistp qword [rel .dummy]

fstsw ax
and rax, 1

hlt
