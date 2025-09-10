%ifdef CONFIG
{
  "RegData": {
    "RAX": "5"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1", "FEX_X87STRICTREDUCEDPRECISION" : "1" }
}
%endif

%include "nan_test_macros.inc"

mov esp, 0xe0000040

; Test x87 signaling NaN round-trip preservation in reduced precision mode
; This test verifies that FLDT -> FSTPT preserves signaling nan across round-trip
; Returns NaN triple: 5 (0b101) for signaling NaN

finit
lea rdx, [rel data]
fld tword [rdx] ; load nan 80bit
fstp tword [rdx + 16] ; store nan as 80bit

; Check the stored 80-bit value using NaN triple macro
lea rax, [rel data + 16]
CHECK_NAN_TRIPLE_80

hlt

align 16
data:
  dq 0xa000000000000000  ; signaling nan significand  
  dw 0x7fff              ; signaling nan exponent
  dw 0, 0, 0             ; padding to 16 bytes
  dq 0, 0                ; space for 80-bit result (16 bytes)

