%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x0000000000000000", "0xFFFFFFFFFFFFFFFF"],
    "XMM1":  ["0x0000000000000000", "0x0000000000000000"],
    "XMM2":  ["0x0000000000000000", "0x0000000000000000"],
    "XMM3":  ["0x0000000000000000", "0xFFFFFFFFFFFFFFFF"],
    "XMM4":  ["0xFFFFFFFFFFFFFFFF", "0x0000000000000000"],
    "XMM5":  ["0xFFFFFFFFFFFFFFFF", "0xFFFFFFFFFFFFFFFF"],
    "XMM6":  ["0x0000000000000000", "0x0000000000000000"],
    "XMM7":  ["0xFFFFFFFFFFFFFFFF", "0x0000000000000000"],
    "XMM8":  ["0x0000000000000000", "0xFFFFFFFFFFFFFFFF"],
    "XMM9":  ["0xFFFFFFFFFFFFFFFF", "0x0000000000000000"]
  },
  "HostFeatures": ["SSE4.2"]
}
%endif

movaps xmm0, [rel .data0]
movaps xmm1, [rel .data1]
movaps xmm2, [rel .data2]
movaps xmm3, [rel .data3]
movaps xmm4, [rel .data4]

movaps xmm5, [rel .data0]
movaps xmm6, [rel .data1]
movaps xmm7, [rel .data2]
movaps xmm8, [rel .data3]
movaps xmm9, [rel .data4]

pcmpgtq xmm0, [rel .data4]
pcmpgtq xmm1, [rel .data3]
pcmpgtq xmm2, [rel .data2]
pcmpgtq xmm3, [rel .data1]
pcmpgtq xmm4, [rel .data0]

pcmpgtq xmm5, [rel .data1]
pcmpgtq xmm6, [rel .data2]
pcmpgtq xmm7, [rel .data3]
pcmpgtq xmm8, [rel .data4]
pcmpgtq xmm9, [rel .data0]

hlt

align 16
.data0:
dq 0
dq 0

.data1:
dq -1
dq -1

.data2:
dq 1
dq 1

.data3:
dq -1
dq 1

.data4:
dq 1
dq -1
