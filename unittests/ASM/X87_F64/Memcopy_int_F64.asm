%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1234567890ABCDEF",
    "RBX": "0x1234567890ABCDEF",
    "RCX": "0x9876543210FEDCBA",
    "RSI": "0x9876543210FEDCBA"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; Test integer memcpy optimization in reduced precision mode
; Large 64-bit integers (> 2^53) should preserve precision when
; fild/fistp operations are optimized to direct memory copy

mov rdx, 0xe0000000

; Test case 1: Large positive integer > 2^53
; 0x1234567890ABCDEF = 1311768467463790319 > 2^53 = 9007199254740992
mov rax, 0x1234567890ABCDEF
mov [rdx + 0], rax

fild qword [rdx + 0]
fistp qword [rdx + 8]

mov rbx, [rdx + 8]

; Test case 2: Large negative integer
; 0x9876543210FEDCBA as signed = -7508735094825308742
mov rcx, 0x9876543210FEDCBA
mov [rdx + 16], rcx

fild qword [rdx + 16]
fistp qword [rdx + 24]

mov rsi, [rdx + 24]

hlt