%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0x0000fdec0000ab3f", "0x00007d7c000036d0", "0xccdbcfc31f3ff0f3", "0x108390defebac4be"],
    "XMM1": ["0x0000312f000006b5", "0x0000115200002d1a", "0", "0"],
    "XMM2": ["0xc4be43cc1ad6970b", "0x4549bd7eb46a1278", "0xf793ef673dac6e4c", "0xbb7b5b3d85d34271"],
    "XMM3": ["0x0000000000000012", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

vmovapd ymm0, [rel .data]
vmovapd ymm1, [rel .data + (1 * 32)]
vmovapd ymm2, [rel .data + (2 * 32)]
vmovaps ymm3, [rel .data + (3 * 32)]

psrld xmm0, 16
vpsrld xmm1, xmm2, 18

hlt

align 32
.data:
dq 0xfdecd28fab3fa4a5, 0x7d7ccd8836d09fc2, 0xccdbcfc31f3ff0f3, 0x108390defebac4be
dq 0x0000000000000010, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000
dq 0xc4be43cc1ad6970b, 0x4549bd7eb46a1278, 0xf793ef673dac6e4c, 0xbb7b5b3d85d34271
dq 0x0000000000000012, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000
