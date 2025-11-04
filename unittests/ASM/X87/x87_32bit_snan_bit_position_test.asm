%ifdef CONFIG
{
  "RegData": {
    "RAX": "6"
  }
}
%endif

%include "nan_test_macros.inc"

mov rsp, 0xe0000040

; Test that a 32-bit signaling NaN gets bit 22 set (not bit 51)
; A 32-bit sNaN has all exponent bits set and bit 22 clear
; After silencing, bit 22 should be set, making it a qNaN
; Value: 0x7F800001 (sNaN) should become 0x7FC00001 (qNaN with bit 22 set)

finit
lea rdx, [rel data]
fld dword [rdx]           ; Load 32-bit sNaN
fstp dword [rdx + 16]     ; Store back - should silence to qNaN

; Load the result and check bit 22 is set
lea rdx, [rdx + 16]
movss xmm0, [rdx]    ; Load 32-bit float into xmm0
CHECK_NAN_TRIPLE_32

hlt

align 4096
data:
  dd 0x7F800001             ; 32-bit signaling NaN (exp=0xFF, frac=1, bit 22 clear)
  dd 0
  dd 0
  dd 0                      ; space for result
