%ifdef CONFIG
{
  "RegData": {
    "RAX": "6"
  }
}
%endif

%include "nan_test_macros.inc"

mov rsp, 0xe0000040

; Test x87 signaling NaN non-preservation in non-reduced precision mode
; This test verifies that FLDT loads a signaling NaN and DOES NOT preserve its signaling nature
; We test that loading a signaling nan preserves it but
; that then storing it as 64bit, transforms it to a quiet nan.
; Returns NaN triple: 6 (0b110) for quiet NaN

finit
lea rdx, [rel data]
fld tword [rdx] ; load snan
fstp qword [rdx + 16] ; store snan as 64bit qnan

; Check the stored 64-bit value using NaN triple macro
lea rdx, [rel data + 16]
movsd xmm0, [rdx]    ; Load 64-bit double into xmm0
CHECK_NAN_TRIPLE_64

hlt

align 8
data:
  dq 0xa000000000000000  ; signaling NaN significand
  dw 0x7fff              ; signaling NaN exponent  
  dq 0                   ; space for 64-bit result