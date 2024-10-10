%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x10"
  }
}
%endif

mov rax, 0
mov rcx, 0x11

jmp .head

.top:

add rax, 1

.head:

loop .top

hlt
