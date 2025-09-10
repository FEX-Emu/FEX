%ifdef CONFIG
{
  "RegData": {
    "RAX": "6"
  }
}
%endif

%include "nan_test_macros.inc"

mov rsp, 0xe0000040

; Test x87 quiet NaN preservation in non-reduced precision mode
; This test verifies that quiet NaNs remain quiet during conversion
; Returns NaN triple: 6 (0b110) for quiet NaN

finit
lea rdx, [rel data]
fld tword [rdx] ; load qnan 80bit
fstp qword [rdx + 16] ; store qnan as 64bit

; Check the stored 64-bit value using NaN triple macro
lea rdx, [rel data + 16]
movsd xmm0, [rdx]    ; Load 64-bit double into xmm0
CHECK_NAN_TRIPLE_64

hlt

align 8
data:
  dq 0xc000000000000000  ; quiet NaN significand
  dw 0x7fff              ; NaN exponent
  dq 0                   ; space for 64-bit result