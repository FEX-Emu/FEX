%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xdeadbeff",
    "RBX": "0xdeadbeff"
  },
  "Mode": "32BIT"
}
%endif

; 0x82 is the 32-bit only alias of the 0x80 group 1 encoding.
; NASM won't emit it, so we have to encode it by hand.
mov eax, 0xdeadbeef

; or al, 0x12
db 0x82, 0xc8, 0x12

mov edi, 0xe0000000
mov dword [edi], 0xdeadbeef

; lock or byte [edi], 0x12
db 0xf0, 0x82, 0x0f, 0x12

mov ebx, [edi]

hlt
