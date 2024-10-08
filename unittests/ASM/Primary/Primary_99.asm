%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFFFFFFFFFFFFF0",
    "RDX": "0xFFFFFFFFFFFFFFFF"
  }
}
%endif

mov rax, 0
mov rdx, 0

mov ax, 0xFFF0
cwd

shl edx, 16
or eax, edx
cdq

shl rdx, 32
or rax, rdx
cqo

hlt
