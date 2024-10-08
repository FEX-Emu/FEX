%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "RAX": "0x00",
    "RBX": "0x03",
    "RDI": "0x00",
    "RSI": "0x33"
  }
}
%endif

lea rdx, [rel .data]

vmovaps ymm0, [rdx]
vmovaps ymm1, [rdx + 32]

vmovmskps rax, xmm0
vmovmskps rbx, xmm1

vmovmskps rdi, ymm0
vmovmskps rsi, ymm1

hlt

align 32
.data:
dq 0x4142434445464748
dq 0x5152535455565758
dq 0x4142434445464748
dq 0x5152535455565758

dq 0x8000000080000000
dq 0x7000000070000000
dq 0x8000000080000000
dq 0x7000000070000000
