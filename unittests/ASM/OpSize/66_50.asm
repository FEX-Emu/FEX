%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x2",
    "RDI": "0x0",
    "XMM0": ["0x0", "0x8000000000000000"]
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x0
mov [rdx + 8 * 0], rax
mov rax, 0x8000000000000000
mov [rdx + 8 * 1], rax

movapd xmm0, [rdx]
movmskpd rax, xmm0

movapd xmm1, [rel .data]
movmskpd rdi, xmm1

hlt

align 16
.data:
dq 0x4142434445464748
dq 0x5152535455565758
