%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0x07ef069405590525", "0x03eb066c01b604fe", "0xccdbcfc31f3ff0f3", "0x108390defebac4be"],
    "XMM1": ["0x018900870035012e", "0x008a017a01680024", "0", "0"],
    "XMM2": ["0xc4be43cc1ad6970b", "0x4549bd7eb46a1278", "0xf793ef673dac6e4c", "0xbb7b5b3d85d34271"],
    "XMM3": ["0x0000000000000007", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

vmovapd ymm0, [rel .data]
vmovapd ymm1, [rel .data + (1 * 32)]
vmovapd ymm2, [rel .data + (2 * 32)]
vmovaps ymm3, [rel .data + (3 * 32)]

psrlw xmm0, xmm1
vpsrlw xmm1, xmm2, xmm3

hlt

align 32
.data:
dq 0xfdecd28fab3fa4a5, 0x7d7ccd8836d09fc2, 0xccdbcfc31f3ff0f3, 0x108390defebac4be
dq 0x0000000000000005, 0x0000000000000000, 0x0004000300020001, 0x0204020302020201
dq 0xc4be43cc1ad6970b, 0x4549bd7eb46a1278, 0xf793ef673dac6e4c, 0xbb7b5b3d85d34271
dq 0x0000000000000007, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000
