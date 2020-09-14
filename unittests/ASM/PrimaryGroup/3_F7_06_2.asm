%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x80000000",
    "RDX": "0"
  }
}
%endif

mov r15, 0xe0000000

mov eax, 0x2
mov [r15 + 8 * 0], eax

mov rax, 0xFFFFFFFF00000000
mov rdx, 0xFFFFFFFF00000001

div dword [r15 + 8 * 0]

hlt
