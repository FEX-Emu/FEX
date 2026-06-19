%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0x43cc1ad6970b4549", "0x0000000000000000", "0xccdbcfc31f3ff0f3", "0x108390defebac4be"]
  }
}
%endif

vmovaps ymm0, [rel .data]
movq mm0, [rel .data + (1 * 32)]

movq2dq xmm0, mm0

hlt

align 32
.data:
dq 0xfdecd28fab3fa4a5, 0x7d7ccd8836d09fc2, 0xccdbcfc31f3ff0f3, 0x108390defebac4be
dq 0x43cc1ad6970b4549, 0xbd7eb46a1278f793, 0xef673dac6e4cbb7b, 0x5b3d85d342718be9
