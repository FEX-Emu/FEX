%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "RAX": "0x0000000000000001",
    "RBX": "0x0000000000000002",
    "RCX": "0x0000000000000003",
    "RDX": "0x0000000000000004",
    "RSI": "0x00000000ffffffff",
    "RDI": "0xfffffffffffffffe",
    "RSP": "0x00000000fffffffd",
    "RBP": "0xfffffffffffffffc"
  }
}
%endif

lea rdx, [rel .data]

vmovapd xmm0, [rdx + 8 * 0]
vmovapd xmm1, [rdx + 8 * 2]

vcvtsd2si eax, xmm0
vcvtsd2si rbx, xmm1

vcvtsd2si ecx, [rdx + 8 * 4]
vcvtsd2si rdx, [rdx + 8 * 6]

vcvtsd2si esi, [rel .data + 8 * 8]
vcvtsd2si rdi, [rel .data + 8 * 10]
vcvtsd2si esp, [rel .data + 8 * 12]
vcvtsd2si rbp, [rel .data + 8 * 14]

hlt

align 32
.data:
dq 0x3FF0000000000000
dq 0x5152535455565758

dq 0x4000000000000000
dq 0x5152535455565758

dq 0x4008000000000000
dq 0x5152535455565758

dq 0x4010000000000000
dq 0x5152535455565758

dq 0xBFF0000000000000
dq 0x5152535455565758

dq 0xC000000000000000
dq 0x5152535455565758

dq 0xC008000000000000
dq 0x5152535455565758

dq 0xC010000000000000
dq 0x5152535455565758
