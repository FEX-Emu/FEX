%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x828486888a8c8e90",
    "RBX": "0x000000008a8c8e90",
    "RCX": "0x4142434445464748",
    "RDX": "0x0000000045464748",
    "R8": "1",
    "R9": "1"
  }
}
%endif

mov rax, 0x4142434445464748
mov rbx, 0x4142434445464748
mov rcx, 0x4142434445464748
mov rdx, 0x4142434445464748
mov r8, 1
mov r9, 1
jmp .test
.test:

; xadd rax, rcx
; Real encoding: 0x48, 0x0f, 0xc1, 0xc8
; For cumulative decode errors, add a REX with all bits set. Will convert rax to r8, and rcx to r9.
db 0x4f, 0x48, 0x0f, 0xc1, 0xc8

; xadd ebx, edx
; Real encoding: 0x0f, 0xc1, 0xd3
; Add a nop-prefix pad between the opcode and full REX.
db 0x4f, 0x2e, 0x0f, 0xc1, 0xd3

hlt
