%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xfa4fa4fa4fa50e8f",
    "RDX": "0x000000000000001c"
  }
}
%endif
; FEX-Emu had a bug where a 128-bit divide with a large unsigned number with a negative number would result in incorrect data.
; This only manifested itself when the sign bit differed between upper and lower halves of the dividend.

mov rax, 0xfffffffffffc70f9
mov rdx, 0x0000000000000000
mov rbx, 0xffffffffffffffd3

jmp .test
.test:
idiv rbx

hlt
