%ifdef CONFIG
{
  "HostFeatures": ["AES", "AVX"],
  "RegData": {
    "XMM0": ["0x67cd07a5b66bc560", "0xb3fddafd015d883b", "0xccdbcfc31f3ff0f3", "0x108390defebac4be"],
    "XMM2": ["0x67cd07a5b66bc560", "0xb3fddafd015d883b", "0", "0"]
  }
}
%endif

vmovapd ymm0, [rel .data]
vmovapd ymm1, [rel .data + 32]
vmovapd ymm2, [rel .data]
vmovapd ymm3, [rel .data + 32]

aesdeclast xmm0, xmm1
vaesdeclast xmm2, xmm2, xmm3

hlt

align 32
.data:
dq 0xfdecd28fab3fa4a5, 0x7d7ccd8836d09fc2, 0xccdbcfc31f3ff0f3, 0x108390defebac4be
dq 0x43cc1ad6970b4549, 0xbd7eb46a1278f793, 0xef673dac6e4cbb7b, 0x5b3d85d342718be9

