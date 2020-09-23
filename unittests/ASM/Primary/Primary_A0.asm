%ifdef CONFIG
{
  "RegData": {
    "RDX": "0x41",
    "RAX": "0x42"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0x200000000
mov rax, 0x41
mov [rdx], rax

mov rdx, 0xe0000000
mov rax, 0x42
mov [rdx], rax

mov rax, -1
; mov rax, [0x200000000]
db 0x48
db 0xA1
dq 0x0000000200000000
mov rdx, rax

mov rax, -1
; mov eax, [0xe0000000]
db 0x67
db 0xA1
dd 0xe0000000

hlt
