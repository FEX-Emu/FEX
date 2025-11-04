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
; This test verifies that FLDT loads a quiet NaN and preserves its nature
; We test that loading a quiet nan preserves it
; that then storing it as 32bit, keeps it as a quiet nan.
; Returns NaN triple: 6 (0b110) for quiet NaN

finit
lea edx, [.data]
fld tword [edx] ; load qnan 80bit
fstp dword [edx + 16] ; store qnan as 32bit

; Check the stored 32-bit value using NaN triple macro
lea edx, [.data + 16]
movss xmm0, [edx]    ; Load 32-bit float into xmm0
CHECK_NAN_TRIPLE_32

hlt

align 4096
.data:
  dq 0xc000000000000000  ; quiet NaN significand
  dw 0x7fff              ; NaN exponent
  dd 0                   ; space for 32-bit result
