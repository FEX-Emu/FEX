%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x00000000FFFFFFFF",
    "RBX": "0x00000000FFFFFFFF",
    "RCX": "0x414243444546FFFF",
    "RDX": "0x414243444546FFFF",
    "RDI": "0x0000000000000001",
    "RSI": "0x0000000000000001",
    "RBP": "0x0",
    "RSP": "0x0"
  }
}
%endif

mov rax, -1
mov rbx, -1

lea rax, [ebx]

mov rbx, -1
mov rcx, -1

lea ebx, [ecx]

mov rcx, 0x4142434445464748
mov rdx, -1

lea cx, [edx]

mov rdx, 0x4142434445464748
mov rdi, -1

lea dx, [rdi]

mov rdi, 0x4142434445464748
mov rsi, 0xFFFFFFFF00000000
mov rbp, 1

lea rdi, [esi + ebp]

mov rsi, 0x4142434445464748
mov rbp, 0xFFFFFFFF00000000
mov rsp, 1

lea esi, [rbp + rsp]

mov rbp, 0x4142434445464748
mov rsp, 0xFFFFFFFF00000000
mov r9,  0x0000000200000000

lea ebp, [esp + r9d]

mov rsp, 0x4142434445464748
mov r9,  0xFFFFFFFF00000000
mov r10, 0x0000000200000000

lea rsp, [r10d + r9d]

hlt
