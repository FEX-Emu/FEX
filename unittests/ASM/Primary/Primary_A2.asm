%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x41",
    "RDX": "0x42"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rax, 0x41
; mov [0x200000000], rax
db 0x48
db 0xA3
dq 0x0000000200000000

mov rax, 0x42
; mov [0xe0000000], eax
db 0x67
db 0xA3
dd 0xe0000000

mov rdx, 0x200000000
mov rax, [rdx]

mov rdx, 0xe0000000
mov edx, [rdx]

hlt
