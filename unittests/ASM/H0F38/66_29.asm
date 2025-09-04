%ifdef CONFIG
{
  "RegData": {
    "XMM1":  ["0xffffffffffffffff", "0xffffffffffffffff"],
    "XMM2":  ["0xffffffffffffffff", "0xffffffffffffffff"],
    "XMM3":  ["0xffffffffffffffff", "0xffffffffffffffff"],
    "XMM4":  ["0xffffffffffffffff", "0xffffffffffffffff"],
    "XMM5":  ["0x0", "0x0"],
    "XMM6":  ["0x0", "0x0"],
    "XMM7":  ["0x0", "0x0"],
    "XMM8":  ["0xffffffffffffffff", "0xffffffffffffffff"],
    "XMM9":  ["0x0", "0xffffffffffffffff"],
    "XMM10": ["0xffffffffffffffff", "0x0"]
  },
  "HostFeatures": ["SSE4.1"]
}
%endif

lea rdx, [rel .data]

movaps xmm1, [rdx + 16 * 0]
movaps xmm2, [rdx + 16 * 1]
movaps xmm3, [rdx + 16 * 2]
movaps xmm4, [rdx + 16 * 3]
movaps xmm5, [rdx + 16 * 4]
movaps xmm6, [rdx + 16 * 5]
movaps xmm7, [rdx + 16 * 6]
movaps xmm8, [rdx + 16 * 7]
movaps xmm9, [rdx + 16 * 0]
movaps xmm10, [rdx + 16 * 0]

pcmpeqq xmm1, [rdx + 16 * 8]
pcmpeqq xmm2, [rdx + 16 * 9]
pcmpeqq xmm3, [rdx + 16 * 10]
pcmpeqq xmm4, [rdx + 16 * 11]
pcmpeqq xmm5, [rdx + 16 * 12]
pcmpeqq xmm6, [rdx + 16 * 13]
pcmpeqq xmm7, [rdx + 16 * 14]
pcmpeqq xmm8, [rdx + 16 * 15]
pcmpeqq xmm9, [rdx + 16 * 16]
pcmpeqq xmm10, [rdx + 16 * 17]

hlt

align 16
.data:
dq 0.0, 0.0
dq 0.0, 1.0
dq 1.0, 0.0
dq 1.0, 1.0
dq 0.0, 0.0
dq 0.0, 1.0
dq 1.0, 0.0
dq 1.0, 1.0

dq 0.0, 0.0
dq 0.0, 1.0
dq 1.0, 0.0
dq 1.0, 1.0
dq 1.0, 1.0
dq 1.0, 0.0
dq 0.0, 1.0
dq 1.0, 1.0
dq 1.0, 0.0
dq 0.0, 1.0
