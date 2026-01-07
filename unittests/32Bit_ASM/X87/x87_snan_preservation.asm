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
; that then storing it as 64bit, transforms it to a quiet nan.
; Returns NaN triple: 6 (0b110) for quiet NaN

finit
mov edx, .data
fld tword [edx] ; load snan
fstp qword [edx + 16] ; store snan as 64bit qnan

; Check the stored 64-bit value using NaN triple macro
mov edx, .data
add edx, 16
CHECK_NAN_TRIPLE_64

hlt

align 4096
.data:
  dq 0xa000000000000000  ; signaling NaN significand
  dw 0x7fff              ; signaling NaN exponent  
  dq 0                   ; space for 64-bit result
