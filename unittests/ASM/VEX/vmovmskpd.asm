%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "RAX": "0x2",
    "RBX": "0xA",
    "RDI": "0x0",
    "RSI": "0x0"
  }
}
%endif

lea rdx, [rel .data]

vmovapd ymm0, [rdx]
vmovapd ymm1, [rdx + 32]

vmovmskpd rax, xmm0
vmovmskpd rbx, ymm0

vmovmskpd rdi, xmm1
vmovmskpd rsi, ymm1

hlt

align 32
.data:
dq 0x0000000000000000
dq 0x8000000000000000
dq 0x0000000000000000
dq 0x8000000000000000

dq 0x4142434445464748
dq 0x5152535455565758
dq 0x4142434445464748
dq 0x5152535455565758
