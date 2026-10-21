%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xdeadbe02"
  },
  "Mode": "32BIT"
}
%endif

; 0x82 is the 32-bit only alias of the 0x80 group 1 encoding.
; NASM won't emit it, so we have to encode it by hand.
mov eax, 0xdeadbeef
stc

; adc al, 0x12
db 0x82, 0xd0, 0x12

hlt
