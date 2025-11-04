%ifdef CONFIG
{
  "RegData": {
    "RAX": "6"
  }
}
%endif

%include "nan_test_macros.inc"

mov rsp, 0xe0000040

; Test x87 signaling NaN silencing by moving through the stack
; Load sNaN, push another value to force stack use, then store the sNaN
; This ensures x87 stack operations are used without arithmetic that would silence

finit
lea rdx, [rel data]

; Load the sNaN
fld dword [rdx]           ; ST(0) = sNaN

; Push another value to force stack management
fld1                      ; ST(0) = 1.0, ST(1) = sNaN

; Exchange to get sNaN back on top
fxch st1                  ; ST(0) = sNaN, ST(1) = 1.0

; Pop the extra value
fstp st1                  ; ST(0) = sNaN (copied to ST(1), then ST(1) popped)

; Store the sNaN - should be silenced to qNaN
fstp dword [rdx + 16]

; Load the result and check bit 22 is set
lea rdx, [rdx + 16]
movss xmm0, [rdx]    ; Load 32-bit float into xmm0
CHECK_NAN_TRIPLE_32

hlt

align 4096
data:
  dd 0x7F800001             ; 32-bit signaling NaN
  dd 0
  dd 0
  dd 0                      ; space for result
