%ifdef CONFIG
{
  "RegData": {
    "RSP": "0xE0000FE8",
    "RBP": "0xE0000FF8"
  }
}
%endif

mov rsp, 0xe0001000
mov rbp, 0xe0001000

enter 0x10, 0
hlt

