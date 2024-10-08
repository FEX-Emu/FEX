%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1",
    "RSP": "0xE0000020"
  }
}
%endif

mov rsp, 0xe0000020
lea rax, [rel .end]
push rax

mov rax, 1
ret
mov rax, 0

.end:
hlt
