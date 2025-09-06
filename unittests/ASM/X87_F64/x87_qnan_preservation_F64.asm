%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x7ff8000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; Test x87 quiet NaN preservation in reduced precision mode
; This test verifies that quiet NaNs remain quiet during conversion

finit
lea rdx, [rel data]
fld tword [rdx] ; load qnan 80bit
fstp qword [rdx + 16] ; store qnan as 64bit
mov rax, [rdx + 16]

hlt

align 8
data:
  dq 0xc000000000000000  ; quiet NaN significand
  dw 0x7fff              ; NaN exponent
  dq 0                   ; space for 64-bit result