%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x41",
    "RDX": "0x42"
  }
}
%endif

mov rax, 0x41
; mov [0xe0000008], rax
db 0x48
db 0xA3
dq 0x00000000e0000008

mov rax, 0x42
; mov [0xe0000000], eax
db 0x67
db 0xA3
dd 0xe0000000

mov rdx, 0xe0000008
mov rax, [rdx]

mov rdx, 0xe0000000
mov edx, [rdx]

hlt
