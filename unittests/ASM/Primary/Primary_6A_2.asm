%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x000000000000FF81",
    "RSP": "0xE0000018"
  }
}
%endif

mov rsp, 0xe0000020

push word 0
push word 0
push word 0
push word -127

mov rdx, 0xe0000020
mov rax, [rdx - 8]

hlt
