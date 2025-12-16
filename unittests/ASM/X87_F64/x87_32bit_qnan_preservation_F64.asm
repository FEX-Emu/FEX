%ifdef CONFIG
{
  "RegData": {
    "RAX": "6"
  },
  "Env": { "FEX_X87STRICTREDUCEDPRECISION" : "1" }
}
%endif

%include "nan_test_macros.inc"

section .text
global _start
_start:

mov esp, 0xe0000040

; Test x87 quiet NaN preservation in reduced precision mode
; This test verifies that quiet NaNs remain quiet during conversion
; Returns NaN triple: 6 (0b110) for quiet NaN

finit
lea rdx, [rel qnan_data]
fld tword [rdx]          ; load qnan 80bit
lea rdx, [rel result]
fstp dword [rdx]         ; store qnan as 32bit

; Check the stored 32-bit value using NaN triple macro
movss xmm0, [rdx]        ; Load 32-bit float into xmm0
CHECK_NAN_TRIPLE_32

hlt

align 4096

section .data
  align 16
  qnan_data:
    dq 0xc000000000000000  ; quiet NaN significand
    dw 0x7fff              ; NaN exponent
  result:
    dd 0                   ; space for 32-bit result
