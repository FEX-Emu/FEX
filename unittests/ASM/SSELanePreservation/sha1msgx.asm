%ifdef CONFIG
{
  "HostFeatures": ["AVX", "SHA"],
  "RegData": {
    "XMM0": ["0x409266e5b9475336", "0x80901f079def3b67", "0xccdbcfc31f3ff0f3", "0x108390defebac4be"],
    "XMM2": ["0xd5cf2f8cea6fa126", "0xde087436ea390a28", "0xccdbcfc31f3ff0f3", "0x108390defebac4be"]
  }
}
%endif

vmovapd ymm0, [rel .data]
vmovapd ymm2, [rel .data]

sha1msg1 xmm0, [rel .data + 32]
sha1msg2 xmm2, [rel .data + 32]

hlt

align 32
.data:
dq 0xfdecd28fab3fa4a5, 0x7d7ccd8836d09fc2, 0xccdbcfc31f3ff0f3, 0x108390defebac4be
dq 0x43cc1ad6970b4549, 0xbd7eb46a1278f793, 0xef673dac6e4cbb7b, 0x5b3d85d342718be9

