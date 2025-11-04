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
; This test verifies that FLDT loads a quiet NaN and preserves its nature
; We test that loading a quiet nan preserves it
; that then storing it as 32bit, keeps it as a quiet nan.
; Returns NaN triple: 6 (0b110) for quiet NaN

finit
lea rdx, [rel data]
fld tword [rdx] ; load qnan 80bit
fstp dword [rdx + 16] ; store qnan as 32bit

; Check the stored 32-bit value using NaN triple macro
lea rdx, [rel data + 16]
movss xmm0, [rdx]    ; Load 32-bit float into xmm0
CHECK_NAN_TRIPLE_32

hlt

align 4096
data:
  dq 0xc000000000000000  ; quiet NaN significand
  dw 0x7fff              ; NaN exponent
  dd 0                   ; space for 32-bit result
