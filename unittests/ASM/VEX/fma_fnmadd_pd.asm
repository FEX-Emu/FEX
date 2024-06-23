%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0x4150000900802300", "0x43ac00013450041a", "0xc0e00000c0c00000", "0x41f0000041a00000"],
    "XMM1": ["0x42400009c1808f73", "0xc5ac0001a728090b", "0x4150000100800300", "0x43ac00013450031a"],
    "XMM2": ["0x423fe01370809e58", "0xc5aa78018bb7185d", "0x4150000100800300", "0x43ac00013450031a"],
    "XMM3": ["0x4150000900802300", "0x43ac00013450041a", "0", "0"],
    "XMM4": ["0x42400009c1808f73", "0xc5ac0001a728090b", "0", "0"],
    "XMM5": ["0x423fe01370809e58", "0xc5aa78018bb7185d", "0", "0"]
  }
}
%endif

vmovups ymm0, [rel .data]
vmovups ymm1, [rel .data2]
vmovups ymm2, [rel .data3]

vmovups ymm3, [rel .data]
vmovups ymm4, [rel .data2]
vmovups ymm5, [rel .data3]

vfnmadd231pd ymm0, ymm1, ymm2
vfnmadd213pd ymm1, ymm0, ymm2
vfnmadd132pd ymm2, ymm1, ymm0

vfnmadd231pd xmm3, xmm4, xmm5
vfnmadd213pd xmm4, xmm3, xmm5
vfnmadd132pd xmm5, xmm4, xmm3

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
