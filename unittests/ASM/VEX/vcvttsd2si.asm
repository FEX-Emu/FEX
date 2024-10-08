%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "RAX": "0x0000000000000001",
    "RBX": "0x0000000000000002",
    "RCX": "0x0000000000000003",
    "RDX": "0x0000000000000004"
  }
}
%endif

lea rdx, [rel .data]

vmovapd xmm0, [rdx + 8 * 0]
vmovapd xmm1, [rdx + 8 * 2]

vcvttsd2si eax, xmm0
vcvttsd2si rbx, xmm1

vcvttsd2si ecx, [rdx + 8 * 4]
vcvttsd2si rdx, [rdx + 8 * 6]

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
