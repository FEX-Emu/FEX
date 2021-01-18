%ifdef CONFIG
{
  "Match": "All",
  "RegData": {
    "RAX": "0x20"
  }
}
%endif

mov rax, 0
cmp rax, 0

jz finish

; multiblock should gracefully handle these invalid ops
db 0xf, 0x3B ; invalid opcode here

finish:
mov rax, 32

hlt