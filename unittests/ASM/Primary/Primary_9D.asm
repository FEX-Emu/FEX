%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x202",
    "RBX": "0x202"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rax, 0x0
mov rbx, 0x0
mov rsp, 0xe0000010

; Setup to default state
mov rax, 0
push rax
popfq

; These pushes will end up being the default rflags initialization value
pushfq

cmp rax, 1
popfq
pushfq

mov rax, qword [rsp]

; These pushes will end up being the default rflags initialization value
db 0x66
pushfq

cmp rax, 1
db 0x66
popfq
db 0x66
pushfq

mov bx, word [rsp]

hlt
