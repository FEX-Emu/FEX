%ifdef CONFIG
{
  "HostFeatures": ["AVX", "SSE4A"],
  "RegData": {
    "XMM0": ["0x0000000000000125", "0x0000000000000000", "0xccdbcfc31f3ff0f3", "0x108390defebac4be"],
    "XMM1": ["0x43cc1ad6970b4549", "0xbd7eb46a1278f793", "0xef673dac6e4cbb7b", "0x5b3d85d342718be9"],
    "XMM2": ["0x0000000000000000", "0x0000000000000000", "0x0000000000003f00", "0x0000000000000000"],
    "XMM3": ["0x000000000000003f", "0x0000000000000000", "0x0000000000003f3f", "0x0000000000000000"]
  }
}
%endif

vmovaps ymm0, [rel .data]
vmovaps ymm1, [rel .data + (1 * 32)]
vmovaps ymm2, [rel .selection]
vmovaps ymm3, [rel .selection + (1 * 32)]

extrq xmm0, xmm1
extrq xmm2, xmm3

hlt

align 32
.data:
dq 0xfdecd28fab3fa4a5, 0x7d7ccd8836d09fc2, 0xccdbcfc31f3ff0f3, 0x108390defebac4be
dq 0x43cc1ad6970b4549, 0xbd7eb46a1278f793, 0xef673dac6e4cbb7b, 0x5b3d85d342718be9
dq 0xc4be43cc1ad6970b, 0x4549bd7eb46a1278, 0xf793ef673dac6e4c, 0xbb7b5b3d85d34271
dq 0x000043cc1ad6970b, 0x4549bd7eb46a1278, 0xf793ef673dac6e4c, 0xbb7b5b3d85d34271

%macro d 2
dq (%1 | (%2 << 8)), 0
%endmacro
align 32
.selection:
d 0, 0
d 0, 63
d 63, 0
d 63, 63
d 0, 31
d 31, 31
d 31, 16
d 48, 8
