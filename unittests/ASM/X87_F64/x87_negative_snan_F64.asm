%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xfff4000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; Test negative x87 signaling NaN preservation in reduced precision mode
; This test verifies that negative signaling NaNs preserve their sign and signaling nature

finit
lea rdx, [rel data]
fld tword [rdx] ; load -snan 80bit
fstp qword [rdx + 16] ; store -snan as 64bit
mov rax, [rdx + 16]

hlt

align 8
data:
  dq 0xa000000000000000  ; signaling nan significand
  dw 0xffff              ; signaling nan exponent (sign bit set)
  dq 0                   ; space for 64-bit result