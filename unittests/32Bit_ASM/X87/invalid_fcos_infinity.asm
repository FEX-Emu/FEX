%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Mode": "32BIT"
}
%endif

section .rodata
    ; Define the 80-bit (10-byte) constant for positive infinity.
    positive_infinity: dt __Infinity__

section .text
global _start
_start:

finit            ; Initialize the FPU
fld tword [rel positive_infinity]  ; Load the constant directly
fcos

fstsw ax
and eax, 1

hlt

