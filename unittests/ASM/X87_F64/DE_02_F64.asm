%ifdef CONFIG
{
  "RegData": {
    "RSI":  "0x18"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rdx, 0xe0000000
mov rsi, 0

; Matching positive-positive
mov rax, 0x3ff0000000000000 ; 1.0
mov [rdx + 8 * 0], rax
mov ax, 1
mov [rdx + 8 * 1], eax

fld qword [rdx + 8 * 0]
ficomp word [rdx + 8 * 1]

; Get the status word
mov rax, 0
fstsw ax
; Extract C3 to see if it was equal
shr ax, 14
and ax, 1
or rsi, rax
shl rsi, 1

; Matching negative-negative
mov rax, 0xbff0000000000000 ; -1.0
mov [rdx + 8 * 0], rax
mov ax, -1
mov [rdx + 8 * 1], eax

fld qword [rdx + 8 * 0]
ficomp word [rdx + 8 * 1]

; Get the status word
mov rax, 0
fstsw ax
; Extract C3 to see if it was equal
shr ax, 14
and ax, 1
or rsi, rax
shl rsi, 1

; Nonmatching negative-positive
mov rax, 0xbff0000000000000 ; -1.0
mov [rdx + 8 * 0], rax
mov ax, 1
mov [rdx + 8 * 1], eax

fld qword [rdx + 8 * 0]
ficomp word [rdx + 8 * 1]

; Get the status word
mov rax, 0
fstsw ax
; Extract C3 to see if it was equal
shr ax, 14
and ax, 1
or rsi, rax
shl rsi, 1

; Nonmatching positive-negative
mov rax, 0x3ff0000000000000 ; 1.0
mov [rdx + 8 * 0], rax
mov ax, -1
mov [rdx + 8 * 1], eax

fld qword [rdx + 8 * 0]
ficomp word [rdx + 8 * 1]

; Get the status word
mov rax, 0
fstsw ax
; Extract C3 to see if it was equal
shr ax, 14
and ax, 1
or rsi, rax
shl rsi, 1

hlt
