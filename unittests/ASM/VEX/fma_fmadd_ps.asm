%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0xc1ac0000c0e00000", "0x4294999942400000", "0xc41f8000c3980000", "0x44bd4000446d0000"],
    "XMM1": ["0x431a0000422e0000", "0xc4291999c3c2c000", "0xc695b300c5be7c00", "0x4793e90d47143780"],
    "XMM2": ["0x429d800042040000", "0xc49c1051c4236000", "0xc5aa2400c4acf000", "0x47eceac0476b3d80"],
    "XMM3": ["0xc1ac0000c0e00000", "0x4294999942400000", "0", "0"],
    "XMM4": ["0x431a0000422e0000", "0xc4291999c3c2c000", "0", "0"],
    "XMM5": ["0x429d800042040000", "0xc49c1051c4236000", "0", "0"]
  }
}
%endif

vmovups ymm0, [rel .data]
vmovups ymm1, [rel .data2]
vmovups ymm2, [rel .data3]

vmovups ymm3, [rel .data]
vmovups ymm4, [rel .data2]
vmovups ymm5, [rel .data3]

vfmadd231ps ymm0, ymm1, ymm2
vfmadd213ps ymm1, ymm0, ymm2
vfmadd132ps ymm2, ymm1, ymm0

vfmadd231ps xmm3, xmm4, xmm5
vfmadd213ps xmm4, xmm3, xmm5
vfmadd132ps xmm5, xmm4, xmm3

hlt

align 32
.data:
dd 2.0, 3.0, 4.0, 5.0
dd 6.0, 7.0, 8.0, 9.0

.data2:
dd -6.0, -7.0, -8.0, -9.0
dd 20.0, 30.0, 40.0, 50.0

.data3:
dd 1.5, 3.5, -5.5, -7.7
dd -15.5, -21.5, 23.5, 30.1
