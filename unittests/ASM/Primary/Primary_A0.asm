%ifdef CONFIG
{
  "RegData": {
    "RDX": "0x41",
    "RAX": "0x42"
  }
}
%endif

mov rdx, 0xe0000008
mov rax, 0x41
mov [rdx], rax

mov rdx, 0xe0000000
mov rax, 0x42
mov [rdx], rax

mov rax, -1
; mov rax, [0xe0000008]
db 0x48
db 0xA1
dq 0x00000000e0000008
mov rdx, rax

mov rax, -1
; mov eax, [0xe0000000]
db 0x67
db 0xA1
dd 0xe0000000

hlt
