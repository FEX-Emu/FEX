%ifdef CONFIG
{
  "HostFeatures": ["AVX", "SHA"],
  "RegData": {
    "XMM0": ["0xa3341202e0256134", "0xce19e96963081f37", "0xccdbcfc31f3ff0f3", "0x108390defebac4be"],
    "XMM2": ["0x8a84294c107f19f3", "0x0f2a42a0a69495b8", "0xccdbcfc31f3ff0f3", "0x108390defebac4be"]
  }
}
%endif

vmovapd ymm0, [rel .data]
vmovapd ymm2, [rel .data]

sha256msg1 xmm0, [rel .data + 32]
sha256msg2 xmm2, [rel .data + 32]

hlt

align 32
.data:
dq 0xfdecd28fab3fa4a5, 0x7d7ccd8836d09fc2, 0xccdbcfc31f3ff0f3, 0x108390defebac4be
dq 0x43cc1ad6970b4549, 0xbd7eb46a1278f793, 0xef673dac6e4cbb7b, 0x5b3d85d342718be9

