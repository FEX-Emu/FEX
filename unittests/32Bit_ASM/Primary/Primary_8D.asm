%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4000",
    "RBX": "0x4000",
    "RCX": "0x8000",
    "RDX": "0x9000",
    "RSI": "0x7FC0",
    "RSP": "0xFFFF7FC0",
    "RBP": "0x1"
  },
  "Mode": "32BIT"
}
%endif

mov eax, 0
mov ebx, 0
mov esp, -1

; Specific encoded `lea ax, [0x4000]`
; Operand size override and address size override
; Nasm doesn't seem to emit this at all
db 0x67, 0x66, 0x8d, 0x06, 0x00, 0x40

lea bx, [0xC000]
lea si, [0x4001]

mov ebp, 0
; Try to LEA past the 16bits
lea ebp, [bx + si]

lea bx, [0x4000]
lea si, [0x4000]

; Address size override and Operand size overrides
lea cx, [bx + si]
lea dx, [bx + si + 0x1000]
lea sp, [bx + si - 64]

; Address size override without operand size override
lea esi, [bx + si - 64]

hlt
