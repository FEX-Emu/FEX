%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0xa000000000000000", "0x7fff"]
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; Test x87 signaling NaN round-trip preservation in reduced precision mode
; This test verifies that FLDT -> FSTPT preserves signaling nan across round-trip

finit
lea rdx, [rel data]
fld tword [rdx] ; load nan 80bit
fstp tword [rdx + 16] ; store nan as 80bit
movups xmm0, [rdx + 16]
hlt

align 16
data:
  dq 0xa000000000000000  ; signaling nan significand  
  dw 0x7fff              ; signaling nan exponent
  dw 0, 0, 0             ; padding to 16 bytes
  dq 0, 0                ; space for 80-bit result (16 bytes)