%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "RAX": "0x0000000000000001",
    "RBX": "0x0000000000000002",
    "RCX": "0x0000000000000003",
    "RDX": "0x0000000000000004",
    "RBP": "0x00000000FFFFFFFE",
    "RSI": "0xFFFFFFFFFFFFFFFC"
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
mov rbp, -1
mov rsi, -1

vcvttss2si eax, xmm0
vcvttss2si rbx, xmm1

vcvttss2si ebp, [r15 + 8 * 8]
vcvttss2si rsi, [r15 + 8 * 10]

vcvttss2si ecx, [r15 + 8 * 4]
vcvttss2si rdx, [r15 + 8 * 6]

hlt

align 32
.data:
dq 0x414243443F800000
dq 0x5152535455565758

dq 0x4142434440000000
dq 0x5152535455565758

dq 0x4142434440400000
dq 0x5152535455565758

dq 0x4142434440800000
dq 0x5152535455565758

dq 0x41424344C0000000
dq 0x5152535455565758

dq 0x41424344C0800000
dq 0x5152535455565758
