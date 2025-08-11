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

o16 enter 0x10, 0
hlt

