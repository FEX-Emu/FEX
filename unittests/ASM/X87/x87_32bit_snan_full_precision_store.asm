%ifdef CONFIG
{
  "RegData": {
    "RAX": "6"
  }
}
%endif

%include "nan_test_macros.inc"

mov rsp, 0xe0000040

; Test x87 signaling NaN silencing in FULL precision mode with 32-bit store
; This test verifies that storing an 80-bit signaling NaN as 32-bit properly
; silences it by setting bit 22 (the quiet bit for 32-bit floats).
;
; The bug: SilenceNaN was hardcoded for 64-bit, using:
; - 64-bit FCmp (won't detect 32-bit NaN as NaN)
; - bit 51 instead of bit 22
; This caused signaling NaNs to remain signaling after 32-bit stores.
;
; Returns NaN triple: 6 (0b110) for quiet NaN (should be converted from signaling)

finit
lea rdx, [rel data]
fld tword [rdx]           ; Load 80-bit signaling NaN
fstp dword [rdx + 16]     ; Store as 32-bit - should silence it

; Check the stored 32-bit value using NaN triple macro
lea rdx, [rel data + 16]
movss xmm0, [rdx]         ; Load 32-bit float into xmm0
CHECK_NAN_TRIPLE_32       ; Should return 6 (quiet NaN)

hlt

align 4096
data:
  dq 0xa000000000000000   ; signaling NaN significand (bit 62 clear)
  dw 0x7fff               ; signaling NaN exponent (all 1s)
  dd 0                    ; space for 32-bit result
