%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0x41dc000041300000", "0xc2809999c2200000", "0x44230000439e0000", "0xc4bb0000c4690000"],
    "XMM1": ["0x4344000042870000", "0xc4129999c3a2c000", "0xc698fb00c5c5fc00", "0x4792270d4711b780"],
    "XMM2": ["0x42c78000424c0000", "0xc4873051c4086000", "0xc5addc00c4b3b000", "0x47ea19da47674580"],
    "XMM3": ["0x41dc000041300000", "0xc2809999c2200000", "0", "0"],
    "XMM4": ["0x4344000042870000", "0xc4129999c3a2c000", "0", "0"],
    "XMM5": ["0x42c78000424c0000", "0xc4873051c4086000", "0", "0"]
  }
}
%endif

vmovups ymm0, [rel .data]
vmovups ymm1, [rel .data2]
vmovups ymm2, [rel .data3]

vmovups ymm3, [rel .data]
vmovups ymm4, [rel .data2]
vmovups ymm5, [rel .data3]

vfnmadd231ps ymm0, ymm1, ymm2
vfnmadd213ps ymm1, ymm0, ymm2
vfnmadd132ps ymm2, ymm1, ymm0

vfnmadd231ps xmm3, xmm4, xmm5
vfnmadd213ps xmm4, xmm3, xmm5
vfnmadd132ps xmm5, xmm4, xmm3

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
