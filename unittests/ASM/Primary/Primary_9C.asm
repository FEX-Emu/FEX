%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x2",
    "RBX": "0x2"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rsp, 0xe0000010

; These pushes will end up being the default rflags initialization value
pushfq

; nasm doesn't encode 16bit pushf
; put the prefix before the instruction manually
db 0x66
pushfq

mov rax, 0x0
mov rbx, 0x0

mov ax, word [rsp]
mov rbx, qword [rsp + 2]

hlt
