%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xdeadbedc"
  },
  "Mode": "32BIT"
}
%endif

; 0x82 is the 32-bit only alias of the 0x80 group 1 encoding.
; NASM won't emit it, so we have to encode it by hand.
mov eax, 0xdeadbeef
stc

; sbb al, 0x12
db 0x82, 0xd8, 0x12

hlt
