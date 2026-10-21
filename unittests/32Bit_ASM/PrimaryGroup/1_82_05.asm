%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xdeadbedd",
    "RBX": "0xdeadbedd"
  },
  "Mode": "32BIT"
}
%endif

; 0x82 is the 32-bit only alias of the 0x80 group 1 encoding.
; NASM won't emit it, so we have to encode it by hand.
mov eax, 0xdeadbeef

; sub al, 0x12
db 0x82, 0xe8, 0x12

mov edi, 0xe0000000
mov dword [edi], 0xdeadbeef

; lock sub byte [edi], 0x12
db 0xf0, 0x82, 0x2f, 0x12

mov ebx, [edi]

hlt
