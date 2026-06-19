%ifdef CONFIG
{
  "HostFeatures": ["AVX", "SSE4A"],
  "RegData": {
    "XMM0": ["0xa4ecd28fab3fa4a5", "0x0000000000000000", "0xccdbcfc31f3ff0f3", "0x108390defebac4be"],
    "XMM1": ["0x43cc1ad6970b4549", "0xbd7eb46a1278f793", "0xef673dac6e4cbb7b", "0x5b3d85d342718be9"],
    "XMM2": ["0x6162636465666768", "0x0000000000000000", "0x5152535455565758", "0x0000000000003f00"],
    "XMM3": ["0x6162636465666768", "0x000000000000003f", "0x7172737475767778", "0x0000000000003f3f"]
  }
}
%endif

vmovaps ymm0, [rel .data]
vmovaps ymm1, [rel .data + (1 * 32)]
vmovaps ymm2, [rel .selection]
vmovaps ymm3, [rel .selection + (1 * 32)]

insertq xmm0, xmm1
insertq xmm2, xmm3

hlt

align 32
.data:
dq 0xfdecd28fab3fa4a5, 0x7d7ccd8836d09fc2, 0xccdbcfc31f3ff0f3, 0x108390defebac4be
dq 0x43cc1ad6970b4549, 0xbd7eb46a1278f793, 0xef673dac6e4cbb7b, 0x5b3d85d342718be9
dq 0xc4be43cc1ad6970b, 0x4549bd7eb46a1278, 0xf793ef673dac6e4c, 0xbb7b5b3d85d34271
dq 0x000043cc1ad6970b, 0x4549bd7eb46a1278, 0xf793ef673dac6e4c, 0xbb7b5b3d85d34271

%macro d 3
dq %3, (%1 | (%2 << 8))
%endmacro
align 32
.selection:
; BitMask, Shift, Data
d 0,  0,  0x4142434445464748
d 0,  63, 0x5152535455565758
d 63, 0,  0x6162636465666768
d 63, 63, 0x7172737475767778
d 0,  31, 0x8182838485868788
d 31, 31, 0x9192939495969798
d 31, 16, 0xA1A2A3A4A5A6A7A8
d 48, 8,  0xB1B2B3B4B5B6B7B8
