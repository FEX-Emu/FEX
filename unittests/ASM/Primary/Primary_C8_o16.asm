%ifdef CONFIG
{
  "RegData": {
    "RSP": "0xE0000FEE",
    "RBP": "0xE0000FFE"
  }
}
%endif

mov rsp, 0xe0001000
mov rbp, 0xe0001000

; operand-size override prefix
; Nasm complains if o16 is used
; `warning: invalid operand size prefix o16, must be o64`
db 0x66
enter 0x10, 0
hlt

