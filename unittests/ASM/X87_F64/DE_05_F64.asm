%ifdef CONFIG
{
  "RegData": {
    "RCX":  "0x3ff0000000000000",
    "RSI":  "0xc008000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3ff0000000000000 ; 1.0
mov [rdx + 8 * 0], rax
mov ax, 2
mov [rdx + 8 * 1], ax

fld qword [rdx + 8 * 0]
fisubr word [rdx + 8 * 1]

fst qword [rdx]
mov rcx, [rdx]

; Test negative
mov rax, 0x3ff0000000000000 ; 1.0
mov [rdx + 8 * 0], rax
mov ax, -2
mov [rdx + 8 * 1], ax

fld qword [rdx + 8 * 0]
fisubr word [rdx + 8 * 1]

fst qword [rdx]
mov rsi, [rdx]

hlt
