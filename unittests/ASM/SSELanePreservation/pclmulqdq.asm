%ifdef CONFIG
{
  "HostFeatures": ["AVX", "PCLMUL"],
  "RegData": {
    "XMM0": ["0x620f544476f3d9cd", "0x3e389dda1e7240ec", "0xccdbcfc31f3ff0f3", "0x108390defebac4be"],
    "XMM2": ["0x620f544476f3d9cd", "0x3e389dda1e7240ec", "0", "0"]
  }
}
%endif

vmovapd ymm0, [rel .data]
vmovapd ymm2, [rel .data]

pclmulqdq xmm0, [rel .data + 32], 0
vpclmulqdq xmm2, xmm2, [rel .data + 32], 0

hlt

align 32
.data:
dq 0xfdecd28fab3fa4a5, 0x7d7ccd8836d09fc2, 0xccdbcfc31f3ff0f3, 0x108390defebac4be
dq 0x43cc1ad6970b4549, 0xbd7eb46a1278f793, 0xef673dac6e4cbb7b, 0x5b3d85d342718be9

