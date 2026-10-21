%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xdeadbeef",
    "RBX": "0x1"
  },
  "Mode": "32BIT"
}
%endif

; 0x82 is the 32-bit only alias of the 0x80 group 1 encoding.
; NASM won't emit it, so we have to encode it by hand.
mov eax, 0xdeadbeef
xor ebx, ebx

; cmp al, 0x12
db 0x82, 0xf8, 0x12

seta bl

hlt
