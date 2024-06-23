%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0xc01c000000000000", "0xc03b800000000000", "0xc073000000000000", "0xc084600000000000"],
    "XMM1": ["0x4045c00000000000", "0x4067a00000000000", "0xc0b7cf8000000000", "0xc0d314a000000000"],
    "XMM2": ["0x4040800000000000", "0xc071d40000000000", "0xc0959e0000000000", "0x40e0629000000000"],
    "XMM3": ["0xc01c000000000000", "0xc03b800000000000", "0", "0"],
    "XMM4": ["0x4045c00000000000", "0x4067a00000000000", "0", "0"],
    "XMM5": ["0x4040800000000000", "0xc071d40000000000", "0", "0"]
  }
}
%endif

vmovups ymm0, [rel .data]
vmovups ymm1, [rel .data2]
vmovups ymm2, [rel .data3]

vmovups ymm3, [rel .data]
vmovups ymm4, [rel .data2]
vmovups ymm5, [rel .data3]

vfmsubadd231pd ymm0, ymm1, ymm2
vfmsubadd213pd ymm1, ymm0, ymm2
vfmsubadd132pd ymm2, ymm1, ymm0

vfmsubadd231pd xmm3, xmm4, xmm5
vfmsubadd213pd xmm4, xmm3, xmm5
vfmsubadd132pd xmm5, xmm4, xmm3

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
