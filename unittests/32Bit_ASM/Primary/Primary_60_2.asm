%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x6",
    "RCX": "0x5",
    "RDX": "0x4",
    "RSP": "0xE0000020",
    "RBX": "0x3",
    "RBP": "0x2",
    "RSI": "0x1",
    "RDI": "0x0"
  },
  "Mode": "32BIT"
}
%endif

mov esp, 0xe0000020

mov eax, 0
mov ecx, 1
mov edx, 2
mov ebx, 3
mov ebp, 4
mov esi, 5
mov edi, 6

; operand-size override prefix
; Nasm complains if o16 is used
; `warning: invalid operand size prefix o16, must be o32`
db 0x66
pusha

; Invert the order
mov ax, [esp + 2 * 0]
mov cx, [esp + 2 * 1]
mov dx, [esp + 2 * 2]
; sp here
mov bx, [esp + 2 * 4]
mov bp, [esp + 2 * 5]
mov si, [esp + 2 * 6]
mov di, [esp + 2 * 7]

; Load sp last
mov sp, [esp + 2 * 3]

hlt
