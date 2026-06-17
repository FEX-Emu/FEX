%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0xffffffffffffffff", "0x0000000000000000", "0xccdbcfc31f3ff0f3", "0x108390defebac4be"],
    "XMM1": ["0xffffffffffffffff", "0x0000000000000000", "0", "0"],
    "XMM2": ["0xc4be43cc1ad6970b", "0x4549bd7eb46a1278", "0xf793ef673dac6e4c", "0xbb7b5b3d85d34271"],
    "XMM3": ["0x000043cc1ad6970b", "0x4549bd7eb46a1278", "0xf793ef673dac6e4c", "0xbb7b5b3d85d34271"]
  }
}
%endif

vmovaps ymm0, [rel .data]
vmovaps ymm1, [rel .data + (1 * 32)]
vmovaps ymm2, [rel .data + (2 * 32)]
vmovaps ymm3, [rel .data + (3 * 32)]

cmppd xmm0, xmm1, 1
vcmppd xmm1, xmm2, xmm3, 1

hlt

align 32
.data:
dq 0xfdecd28fab3fa4a5, 0x7d7ccd8836d09fc2, 0xccdbcfc31f3ff0f3, 0x108390defebac4be
dq 0x43cc1ad6970b4549, 0xbd7eb46a1278f793, 0xef673dac6e4cbb7b, 0x5b3d85d342718be9
dq 0xc4be43cc1ad6970b, 0x4549bd7eb46a1278, 0xf793ef673dac6e4c, 0xbb7b5b3d85d34271
dq 0x000043cc1ad6970b, 0x4549bd7eb46a1278, 0xf793ef673dac6e4c, 0xbb7b5b3d85d34271
