%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

; Test fsin(+infinity) in reduced precision mode = Invalid Operation (should set bit 0 of status word)
finit
; Set reduced precision mode (64-bit)
fnstcw [rel .cw]
mov ax, [rel .cw]
and ax, 0xFCFF ; Clear precision bits (bits 8-9)
or ax, 0x0200  ; Set to 64-bit precision (10b)
mov [rel .cw], ax
fldcw [rel .cw]

; Load positive infinity: IEEE 754 double precision infinity
mov rax, 0x7FF0000000000000
mov [rel .pos_inf], rax

fld qword [rel .pos_inf]
fsin

fstsw ax
and rax, 1

hlt

.cw:
dw 0

.pos_inf:
dq 0
