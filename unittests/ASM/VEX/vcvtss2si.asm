%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "RAX": "0x0000000000000001",
    "RBX": "0xFFFFFFFFFFFFFFFF",
    "RCX": "0x00000000FFFFFFFE",
    "RDX": "0xFFFFFFFFFFFFFFFC"
  }
}
%endif

lea r15, [rel .data]

vmovapd xmm0, [r15 + 8 * 0]
vmovapd xmm1, [r15 + 8 * 2]

mov rax, -1
mov rbx, -1
mov rcx, -1
mov rdx, -1

vcvtss2si eax, xmm0
vcvtss2si rbx, xmm1

vcvtss2si ecx, [r15 + 8 * 4]
vcvtss2si rdx, [r15 + 8 * 6]

hlt

align 32
.data:
dq 0x414243443F800000
dq 0x5152535455565758

dq 0x41424344BF800000
dq 0x5152535455565758

dq 0x41424344C0000000
dq 0x5152535455565758

dq 0x41424344C0800000
dq 0x5152535455565758
