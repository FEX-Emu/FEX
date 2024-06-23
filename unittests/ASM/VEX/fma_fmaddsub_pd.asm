%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0xc026000000000000", "0xc035800000000000", "0xc073c00000000000", "0xc083f00000000000"],
    "XMM1": ["0x4050200000000000", "0x4063400000000000", "0xc0b8a08000000000", "0xc0d2b66000000000"],
    "XMM2": ["0xc054400000000000", "0x4053b00000000000", "0x40c5e14000000000", "0xc0b5448000000000"],
    "XMM3": ["0xc026000000000000", "0xc035800000000000", "0", "0"],
    "XMM4": ["0x4050200000000000", "0x4063400000000000", "0", "0"],
    "XMM5": ["0xc054400000000000", "0x4053b00000000000", "0", "0"]
  }
}
%endif

vmovups ymm0, [rel .data]
vmovups ymm1, [rel .data2]
vmovups ymm2, [rel .data3]

vmovups ymm3, [rel .data]
vmovups ymm4, [rel .data2]
vmovups ymm5, [rel .data3]

vfmaddsub231pd ymm0, ymm1, ymm2
vfmaddsub213pd ymm1, ymm0, ymm2
vfmaddsub132pd ymm2, ymm1, ymm0

vfmaddsub231pd xmm3, xmm4, xmm5
vfmaddsub213pd xmm4, xmm3, xmm5
vfmaddsub132pd xmm5, xmm4, xmm3

hlt

align 32
.data:
dq 2.0, 3.0
dq 6.0, 7.0

.data2:
dq -6.0, -7.0
dq 20.0, 30.0

.data3:
dq 1.5, 3.5
dq -15.5, -21.5
