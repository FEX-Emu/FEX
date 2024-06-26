%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0xc01c000000000000", "0xc035800000000000", "0xc073000000000000", "0xc083f00000000000"],
    "XMM1": ["0x4045c00000000000", "0x4063400000000000", "0xc0b7cf8000000000", "0xc0d2b66000000000"],
    "XMM2": ["0x4040800000000000", "0x4053b00000000000", "0xc0959e0000000000", "0xc0b5448000000000"],
    "XMM3": ["0xc01c000000000000", "0xc035800000000000", "0", "0"],
    "XMM4": ["0x4045c00000000000", "0x4063400000000000", "0", "0"],
    "XMM5": ["0x4040800000000000", "0x4053b00000000000", "0", "0"]
  }
}
%endif

vmovups ymm0, [rel .data]
vmovups ymm1, [rel .data2]
vmovups ymm2, [rel .data3]

vmovups ymm3, [rel .data]
vmovups ymm4, [rel .data2]
vmovups ymm5, [rel .data3]

vfmadd231pd ymm0, ymm1, ymm2
vfmadd213pd ymm1, ymm0, ymm2
vfmadd132pd ymm2, ymm1, ymm0

vfmadd231pd xmm3, xmm4, xmm5
vfmadd213pd xmm4, xmm3, xmm5
vfmadd132pd xmm5, xmm4, xmm3

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
