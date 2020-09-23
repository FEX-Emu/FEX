%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464748",
    "RBX": "0x5152535455565758",
    "RCX": "0x41424344FFFFFFFF",
    "RSP": "0x51525354FFFFFFFF",
    "RSI": "0xFFFFFFFFFFFFFFFF",
    "RDI": "0xFFFFFFFFFFFFFFFF"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

mov rax, 0x8080808080808080
mov [rdx + 8 * 2], rax
mov [rdx + 8 * 3], rax

mov rax, 0x8080808000000000
mov [rdx + 8 * 4], rax
mov [rdx + 8 * 5], rax

mov rax, 0
mov [rdx + 8 * 6], rax
mov [rdx + 8 * 7], rax

mov rax, -1
mov [rdx + 8 * 8], rax
mov [rdx + 8 * 9], rax

mov [rdx + 8 * 10], rax
mov [rdx + 8 * 11], rax

mov [rdx + 8 * 12], rax
mov [rdx + 8 * 13], rax

movaps xmm0, [rdx + 8 * 0]
movaps xmm1, [rdx + 8 * 2]
movaps xmm2, [rdx + 8 * 4]
movaps xmm3, [rdx + 8 * 6]

lea rdi, [rdx + 8 * 8]
maskmovdqu xmm0, xmm1

lea rdi, [rdx + 8 * 10]
maskmovdqu xmm0, xmm2

lea rdi, [rdx + 8 * 12]
maskmovdqu xmm0, xmm3

mov rax, qword [rdx + 8 * 8]
mov rbx, qword [rdx + 8 * 9]

mov rcx, qword [rdx + 8 * 10]
mov rsp, qword [rdx + 8 * 11]

mov rsi, qword [rdx + 8 * 12]
mov rdi, qword [rdx + 8 * 13]

hlt
