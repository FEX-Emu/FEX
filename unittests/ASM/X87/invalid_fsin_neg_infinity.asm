%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

; Test fsin(-infinity) = Invalid Operation (should set bit 0 of status word)
; Load negative infinity: exponent all 1s, mantissa 0x8000000000000000, sign bit set
mov rax, 0x8000000000000000
mov [rel .neg_inf], rax
mov ax, 0xFFFF
mov [rel .neg_inf + 8], ax

fld tword [rel .neg_inf]
fsin

fstsw ax
and rax, 1

hlt

.neg_inf:
dq 0
dw 0
