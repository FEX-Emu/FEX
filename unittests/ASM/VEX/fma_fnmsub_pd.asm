%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0x414ffff200ffc600", "0x43ac00013450021a", "0x40e00000c0c00000", "0xc1f0000041a00000"],
    "XMM1": ["0x423ffff381ff1d62", "0xc5ac0001a728070b", "0xc150000100800300", "0xc3ac00013450031a"],
    "XMM2": ["0xc2400ff9ba3fce78", "0x45ad8801c298f79d", "0x4150000100800300", "0x43ac00013450031a"],
    "XMM3": ["0x414ffff200ffc600", "0x43ac00013450021a", "0", "0"],
    "XMM4": ["0x423ffff381ff1d62", "0xc5ac0001a728070b", "0", "0"],
    "XMM5": ["0xc2400ff9ba3fce78", "0x45ad8801c298f79d", "0", "0"]
  }
}
%endif

vmovups ymm0, [rel .data]
vmovups ymm1, [rel .data2]
vmovups ymm2, [rel .data3]

vmovups ymm3, [rel .data]
vmovups ymm4, [rel .data2]
vmovups ymm5, [rel .data3]

vfnmsub231pd ymm0, ymm1, ymm2
vfnmsub213pd ymm1, ymm0, ymm2
vfnmsub132pd ymm2, ymm1, ymm0

vfnmsub231pd xmm3, xmm4, xmm5
vfnmsub213pd xmm4, xmm3, xmm5
vfnmsub132pd xmm5, xmm4, xmm3

hlt

align 32
.data:
dd 2.0, 3.0
dd 6.0, 7.0

.data2:
dd -6.0, -7.0
dd 20.0, 30.0

.data3:
dd 1.5, 3.5
dd -15.5, -21.5
