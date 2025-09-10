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

; Test x87 signaling NaN non-preservation in non-reduced precision mode (32-bit)
; This test verifies that FLDT loads a signaling NaN and DOES NOT preserve its signaling nature
; We test that loading a signaling nan preserves it but
; that then storing it as 32bit, transforms it to a quiet nan.
; Returns NaN triple: 6 (0b110) for quiet NaN (converted from signaling)

finit
lea edx, [.data]
fld tword [edx] ; load snan
fstp dword [edx + 16] ; store snan as 32bit qnan

; Check the stored 32-bit value using NaN triple macro
lea edx, [.data + 16]
movss xmm0, [edx]    ; Load 32-bit float into xmm0
CHECK_NAN_TRIPLE_32

hlt

align 8
.data:
  dq 0xa000000000000000  ; signaling NaN significand
  dw 0x7fff              ; signaling NaN exponent  
  dd 0                   ; space for 32-bit result