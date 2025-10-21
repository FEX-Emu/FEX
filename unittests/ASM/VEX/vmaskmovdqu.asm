%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
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

lea rdx, [rel .data]

vmovaps xmm0, [rdx + 16 * 0]
vmovaps xmm1, [rdx + 16 * 1]
vmovaps xmm2, [rdx + 16 * 2]
vmovaps xmm3, [rdx + 16 * 3]

lea rdi, [rdx + 16 * 4]
vmaskmovdqu xmm0, xmm1

lea rdi, [rdx + 16 * 5]
vmaskmovdqu xmm0, xmm2

lea rdi, [rdx + 16 * 6]
vmaskmovdqu xmm0, xmm3

mov rax, qword [rdx + 8 * 8]
mov rbx, qword [rdx + 8 * 9]

mov rcx, qword [rdx + 8 * 10]
mov rsp, qword [rdx + 8 * 11]

mov rsi, qword [rdx + 8 * 12]
mov rdi, qword [rdx + 8 * 13]

hlt

align 4096
.data:
dq 0x4142434445464748
dq 0x5152535455565758

dq 0x8080808080808080
dq 0x8080808080808080

dq 0x8080808000000000
dq 0x8080808000000000

dq 0x0000000000000000
dq 0x0000000000000000

dq -1
dq -1

dq -1
dq -1

dq -1
dq -1
