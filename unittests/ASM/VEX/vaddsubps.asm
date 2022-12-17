%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM2": ["0x41200000C0000000", "0x41200000C0C00000", "0x0000000000000000", "0x0000000000000000"],
    "XMM3": ["0x41200000C0000000", "0x41200000C0C00000", "0x41200000C0000000", "0x41200000C0C00000"],
    "XMM4": ["0x41200000C0000000", "0x41200000C0C00000", "0x0000000000000000", "0x0000000000000000"],
    "XMM5": ["0x41200000C0000000", "0x41200000C0C00000", "0x41200000C0000000", "0x41200000C0C00000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

lea rdx, [rel .data]

vmovapd ymm0, [rdx]
vmovapd ymm1, [rdx + 32]

vaddsubps xmm2, xmm0, [rdx + 32]
vaddsubps ymm3, ymm0, [rdx + 32]

vaddsubps xmm4, xmm0, xmm1
vaddsubps ymm5, ymm0, ymm1

hlt

align 32
.data:
dq 0x4080000040400000 ; 4, 3
dq 0x400000003f800000 ; 2, 1
dq 0x4080000040400000 ; 4, 3
dq 0x400000003f800000 ; 2, 1

dq 0x40C0000040A00000 ; 6, 5
dq 0x4100000040E00000 ; 8, 7
dq 0x40C0000040A00000 ; 6, 5
dq 0x4100000040E00000 ; 8, 7
