%ifdef CONFIG
{
  "RegData": {
    "RAX": "5"
  }
}
%endif

%include "nan_test_macros.inc"

mov rsp, 0xe0000040

; Test x87 signaling negative NaN round-trip preservation in non-reduced precision mode
; This test verifies that FLDT -> FSTPT preserves signaling nan across round-trip
; Returns NaN triple: 5 (0b101) for signaling NaN

finit
lea rdx, [rel data]
fld tword [rdx] ; load snan 80bit
fstp tword [rdx + 16] ; store nan as 80bit

; Check the stored 80-bit value using NaN triple macro
lea rax, [rel data + 16]
CHECK_NAN_TRIPLE_80

hlt

align 16
data:
  dq 0xa000000000000000  ; signaling nan significand  
  dw 0xffff              ; signaling nan exponent
  dw 0, 0, 0             ; padding to 16 bytes
  dq 0, 0                ; space for 80-bit result (16 bytes)