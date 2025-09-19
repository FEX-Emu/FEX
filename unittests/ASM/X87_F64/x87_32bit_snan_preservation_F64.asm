%ifdef CONFIG
{
  "RegData": {
    "RAX": "6"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1", "FEX_X87STRICTREDUCEDPRECISION" : "1" }
}
%endif

%include "nan_test_macros.inc"

mov esp, 0xe0000040

; Test x87 signaling NaN non-preservation in reduced precision mode
; This test verifies that FLDT loads a signaling NaN and DOES NOT preserve its signaling nature
; We test that loading a signaling nan preserves it but
; that then storing it as 32bit, transforms it to a quiet nan.
; Returns NaN triple: 6 (0b110) for quiet NaN (converted from signaling)

finit
lea rdx, [rel data]
fld tword [rdx] ; load snan
fstp dword [rdx + 16] ; store snan as 32bit qnan

; Check the stored 32-bit value using NaN triple macro
lea rdx, [rel data + 16]
movss xmm0, [rdx]    ; Load 32-bit float into xmm0
CHECK_NAN_TRIPLE_32

hlt

align 8
data:
  dq 0xa000000000000000  ; signaling NaN significand
  dw 0x7fff              ; signaling NaN exponent  
  dd 0                   ; space for 32-bit result

