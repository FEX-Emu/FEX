%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0xfdecd28f3f800000", "0x7d7ccd8836d09fc2", "0xccdbcfc31f3ff0f3", "0x108390defebac4be"],
    "XMM1": ["0xc4be43cc3f800000", "0x4549bd7eb46a1278", "0", "0"],
    "XMM2": ["0xc4be43cc3f800000", "0x4549bd7eb46a1278", "0xf793ef673dac6e4c", "0xbb7b5b3d85d34271"],
    "XMM3": ["0x000043cc40000000", "0x4549bd7eb46a1278", "0xf793ef673dac6e4c", "0xbb7b5b3d85d34271"]
  }
}
%endif

vmovaps ymm0, [rel .data]
vmovaps ymm1, [rel .data + (1 * 32)]
vmovaps ymm2, [rel .data + (2 * 32)]
vmovaps ymm3, [rel .data + (3 * 32)]

minss xmm0, xmm1
vminss xmm1, xmm2, xmm3

hlt

align 32
.data:
dq 0xfdecd28f3f800000, 0x7d7ccd8836d09fc2, 0xccdbcfc31f3ff0f3, 0x108390defebac4be
dq 0x43cc1ad640000000, 0xbd7eb46a1278f793, 0xef673dac6e4cbb7b, 0x5b3d85d342718be9
dq 0xc4be43cc3f800000, 0x4549bd7eb46a1278, 0xf793ef673dac6e4c, 0xbb7b5b3d85d34271
dq 0x000043cc40000000, 0x4549bd7eb46a1278, 0xf793ef673dac6e4c, 0xbb7b5b3d85d34271
