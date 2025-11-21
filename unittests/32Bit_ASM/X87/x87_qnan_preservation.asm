%ifdef CONFIG
{
  "RegData": {
    "RAX": "6"
  },
  "Mode": "32BIT"
}
%endif

%include "nan_test_macros.inc"

mov esp, 0xe0000040

; Test x87 quiet NaN preservation in non-reduced precision mode (32-bit)
; This test verifies that quiet NaNs remain quiet during conversion
; Returns NaN triple: 6 (0b110) for quiet NaN

finit
lea edx, [.data]
fld tword [edx] ; load qnan 80bit
fstp qword [edx + 16] ; store qnan as 64bit

; Check the stored 64-bit value using NaN triple macro
lea edx, [.data + 16]
movsd xmm0, [edx]    ; Load 64-bit double into xmm0
CHECK_NAN_TRIPLE_64

hlt

align 4096
.data:
  dq 0xc000000000000000  ; quiet NaN significand
  dw 0x7fff              ; NaN exponent
  dq 0                   ; space for 64-bit result
