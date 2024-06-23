%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0x41ac000040e00000", "0xc2949999c2400000", "0x441f800043980000", "0xc4bd4000c46d0000"],
    "XMM1": ["0x4313000042220000", "0xc4253fffc3bd4000", "0xc6955d00c5bd8400", "0x4793caf347140880"],
    "XMM2": ["0xc35e4000c24c0000", "0x42b1c7ad42e50000", "0x4700438046286200", "0xc6eb24ffc6740a00"],
    "XMM3": ["0x41ac000040e00000", "0xc2949999c2400000", "0", "0"],
    "XMM4": ["0x4313000042220000", "0xc4253fffc3bd4000", "0", "0"],
    "XMM5": ["0xc35e4000c24c0000", "0x42b1c7ad42e50000", "0", "0"]
  }
}
%endif

vmovups ymm0, [rel .data]
vmovups ymm1, [rel .data2]
vmovups ymm2, [rel .data3]

vmovups ymm3, [rel .data]
vmovups ymm4, [rel .data2]
vmovups ymm5, [rel .data3]

vfnmsub231ps ymm0, ymm1, ymm2
vfnmsub213ps ymm1, ymm0, ymm2
vfnmsub132ps ymm2, ymm1, ymm0

vfnmsub231ps xmm3, xmm4, xmm5
vfnmsub213ps xmm4, xmm3, xmm5
vfnmsub132ps xmm5, xmm4, xmm3

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
