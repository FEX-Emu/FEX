%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0xbd8051e067e094a0", "0xaf80b100da00f840", "0xccdbcfc31f3ff0f3", "0x108390defebac4be"],
    "XMM1": ["0x5f00e6006b008580", "0xa480bf0035003c00", "0", "0"],
    "XMM2": ["0xc4be43cc1ad6970b", "0x4549bd7eb46a1278", "0xf793ef673dac6e4c", "0xbb7b5b3d85d34271"],
    "XMM3": ["0x0000000000000007", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

vmovapd ymm0, [rel .data]
vmovapd ymm1, [rel .data + (1 * 32)]
vmovapd ymm2, [rel .data + (2 * 32)]
vmovaps ymm3, [rel .data + (3 * 32)]

psllw xmm0, xmm1
vpsllw xmm1, xmm2, xmm3

hlt

align 32
.data:
dq 0xfdecd28fab3fa4a5, 0x7d7ccd8836d09fc2, 0xccdbcfc31f3ff0f3, 0x108390defebac4be
dq 0x0000000000000005, 0x0000000000000000, 0x0004000300020001, 0x0204020302020201
dq 0xc4be43cc1ad6970b, 0x4549bd7eb46a1278, 0xf793ef673dac6e4c, 0xbb7b5b3d85d34271
dq 0x0000000000000007, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000
