%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM10": ["0x20F9B0697CD37844", "0x3B8E6EAE8C165248", "0x20F9B0697CD37844", "0x3B8E6EAE8C165248"],
    "XMM11": ["0x1ED685B8691D35CA", "0x5DAE74E0AB7B51E2", "0x1ED685B8691D35CA", "0x5DAE74E0AB7B51E2"]
  }
}
%endif

; Small test that ensures aliasing source/dest is handled properly.

lea rdx, [rel .data]

vmovapd ymm6, [rdx + 32]
vmovapd ymm7, [rdx]

; 256-bit register only
vmovapd ymm10, ymm7
vpavgw ymm10, ymm10, ymm6

vmovapd ymm11, [rdx + 64]
vpavgw ymm11, ymm7, ymm11

hlt

align 32
.data:
dq 0x2BB883523D4F3197
dq 0x1246C77764260189
dq 0x2BB883523D4F3197
dq 0x1246C77764260189

dq 0x163ADD80BC57BEF1
dq 0x64D615E5B405A306
dq 0x163ADD80BC57BEF1
dq 0x64D615E5B405A306

dq 0x11F4881D94EB39FC
dq 0xA9162248F2D0A23A
dq 0x11F4881D94EB39FC
dq 0xA9162248F2D0A23A
