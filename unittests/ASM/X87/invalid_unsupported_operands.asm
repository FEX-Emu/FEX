%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xC000000000000000",
    "RBX": "0xFFFF",
    "RCX": "0xC000000000000000",
    "RDX": "0xFFFF",
    "RSI": "0xC000000000000000",
    "RDI": "0xFFFFC000"
  }
}
%endif

; Test x87 behavior when operands are in unsupported formats (Pseudo-NaN, Pseudo-Infinity, Unnormal)
; As well as FBSTP producing Packed BCD Indefinite.
; Intel SDM specifies that unsupported operands trigger Invalid Operation exception (#IA)
; and return QNaN Indefinite (Sign=1, Exp=0x7FFF, Significand=0xC000000000000000).
; FBSTP stores the Packed BCD Indefinite (0xFFFFC000000000000000).

fninit

; 1. FSIN with Unnormal operand (Exp=0x3FFF, Significand=0x7FFFFFFFFFFFFFFFULL - bit 63 is 0)
fld tword [rel .unnormal_op]
fsin
fstp tword [rel .res_fsin]

; 2. F2XM1 with Pseudo-NaN operand (Exp=0x7FFF, Significand=0x0000000000000001ULL - bit 63 is 0)
fld tword [rel .pseudonan_op]
f2xm1
fstp tword [rel .res_f2xm1]

; 3. FBSTP with Unnormal operand -> must store Packed BCD Indefinite
fld tword [rel .unnormal_op]
fbstp [rel .res_fbstp]

; Load results to registers for verification
; FSIN result (QNaN Indefinite):
mov rax, [rel .res_fsin]        ; 0xC000000000000000
movzx rbx, word [rel .res_fsin + 8] ; 0xFFFF

; F2XM1 result (QNaN Indefinite):
mov rcx, [rel .res_f2xm1]       ; 0xC000000000000000
movzx rdx, word [rel .res_f2xm1 + 8] ; 0xFFFF

; FBSTP result (Packed BCD Indefinite):
; Bytes 0..6: 0x00, byte 7: 0xC0 (making low 8-bytes 0xC000000000000000), bytes 8..9: 0xFF, 0xFF
mov rsi, [rel .res_fbstp]       ; 0x0
mov edi, dword [rel .res_fbstp + 6] ; upper bytes: byte 8 is 0xC0, byte 9 is 0xFF -> 0xFFFFC000

hlt

align 4096
.unnormal_op:
dq 0x7FFFFFFFFFFFFFFF
dw 0x3FFF ; Exponent 0x3FFF, bit 63 is 0 => Unnormal

.pseudonan_op:
dq 0x0000000000000001
dw 0x7FFF ; Exponent 0x7FFF, bit 63 is 0 => Pseudo-NaN

.res_fsin:
dq 0
dw 0

.res_f2xm1:
dq 0
dw 0

.res_fbstp:
dq 0
dw 0
