%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x7ff4000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; Test x87 signaling NaN preservation in reduced precision mode
; This test verifies that FLDT loads a signaling NaN and preserves its signaling nature

finit
lea rdx, [rel data]
fld tword [rdx] ; load snan
fstp qword [rdx + 16] ; store snan as 64bit
mov rax, [rdx + 16]
hlt

align 8
data:
  dq 0xa000000000000000  ; signaling NaN significand
  dw 0x7fff              ; signaling NaN exponent  
  dq 0                   ; space for 64-bit result