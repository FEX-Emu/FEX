%ifdef CONFIG
{
  "RegData": {
    "RAX": "1",
    "RBX": "1"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

section .data
.dummy: dq 0x0

section .text
global _start

_start:
; Test FIST with NaN = Invalid Operation (should set bit 0 of status word) - reduced precision
; Create NaN by 0/0
fldz
fldz
fdiv

fstsw ax
and rax, 1
mov rbx, rax

; Try to convert NaN to integer - this should be invalid
fistp dword [rel .dummy]

fstsw ax
and rax, 1

hlt
