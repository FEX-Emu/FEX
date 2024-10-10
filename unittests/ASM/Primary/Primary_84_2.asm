%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0"
  }
}
%endif

mov rdx, 0xe0000000
mov dword [rdx], 0xFFFFFF00

mov     r11d, dword[rdx]
test    r11b, r11b
jnz     notzero

mov rax, 0x0
hlt

notzero:
mov rax, 0x1
hlt