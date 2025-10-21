%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Mode": "32BIT"
}
%endif

; Test fsincos(+infinity) in 32-bit mode = Invalid Operation (should set bit 0 of status word)
; Load positive infinity: exponent all 1s, mantissa 0x8000000000000000
mov eax, 0x00000000
mov [rel .pos_inf], eax
mov eax, 0x80000000
mov [rel .pos_inf + 4], eax
mov ax, 0x7FFF
mov [rel .pos_inf + 8], ax

fld tword [rel .pos_inf]
fsincos

fstsw ax
and eax, 1

hlt

align 4096
.pos_inf:
dq 0
dw 0
