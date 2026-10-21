%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xdeadbefd",
    "RBX": "0xdeadbefd"
  },
  "Mode": "32BIT"
}
%endif

; 0x82 is the 32-bit only alias of the 0x80 group 1 encoding.
; NASM won't emit it, so we have to encode it by hand.
mov eax, 0xdeadbeef

; xor al, 0x12
db 0x82, 0xf0, 0x12

mov edi, 0xe0000000
mov dword [edi], 0xdeadbeef

; lock xor byte [edi], 0x12
db 0xf0, 0x82, 0x37, 0x12

mov ebx, [edi]

hlt
