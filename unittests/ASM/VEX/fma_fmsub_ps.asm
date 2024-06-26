%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0xc1dc0000c1300000", "0x4280999942200000", "0xc4230000c39e0000", "0x44bb000044690000"],
    "XMM1": ["0x433d000042810000", "0xc40ebfffc39d4000", "0xc698a500c5c50400", "0x479208f347118880"],
    "XMM2": ["0xc38ea000c2a20000", "0x4297c7ac42bd0000", "0x47031480462f0a00", "0xc6e85899c66fea00"],
    "XMM3": ["0xc1dc0000c1300000", "0x4280999942200000", "0", "0"],
    "XMM4": ["0x433d000042810000", "0xc40ebfffc39d4000", "0", "0"],
    "XMM5": ["0xc38ea000c2a20000", "0x4297c7ac42bd0000", "0", "0"]
  }
}
%endif

vmovups ymm0, [rel .data]
vmovups ymm1, [rel .data2]
vmovups ymm2, [rel .data3]

vmovups ymm3, [rel .data]
vmovups ymm4, [rel .data2]
vmovups ymm5, [rel .data3]

vfmsub231ps ymm0, ymm1, ymm2
vfmsub213ps ymm1, ymm0, ymm2
vfmsub132ps ymm2, ymm1, ymm0

vfmsub231ps xmm3, xmm4, xmm5
vfmsub213ps xmm4, xmm3, xmm5
vfmsub132ps xmm5, xmm4, xmm3

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
