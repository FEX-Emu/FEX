%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "RBX": "12346"
  }
}
%endif

section .rodata
.value: dq 12345.75

section .bss
.result: resw 1

section .text
global _start

_start:

; Test FIST with valid 16-bit conversion
; Load a value that fits in int16 range

finit
fld qword [rel .value]

; Convert to int16 - this should work without overflow
fistp word [rel .result]

fstsw ax
and rax, 1

; Load the result to verify conversion worked
movzx rbx, word [rel .result]

hlt
