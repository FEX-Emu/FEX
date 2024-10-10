%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0", "0", "0", "0"],
    "XMM1": ["0", "0", "0", "0"],
    "XMM2": ["0", "0", "0", "0"],
    "XMM3": ["0", "0", "0", "0"],
    "XMM4": ["0", "0", "0", "0"],
    "XMM5": ["0", "0", "0", "0"],
    "XMM6": ["0", "0", "0", "0"],
    "XMM7": ["0", "0", "0", "0"],
    "XMM8": ["0", "0", "0", "0"],
    "XMM9": ["0", "0", "0", "0"],
    "XMM10": ["0", "0", "0", "0"],
    "XMM11": ["0", "0", "0", "0"],
    "XMM12": ["0", "0", "0", "0"],
    "XMM13": ["0", "0", "0", "0"],
    "XMM14": ["0", "0", "0", "0"],
    "XMM15": ["0", "0", "0", "0"]
  }
}
%endif

lea rdx, [rel .data]

vmovapd ymm0, [rdx]
vmovapd ymm1, [rdx]
vmovapd ymm2, [rdx]
vmovapd ymm3, [rdx]
vmovapd ymm4, [rdx]
vmovapd ymm5, [rdx]
vmovapd ymm6, [rdx]
vmovapd ymm7, [rdx]
vmovapd ymm8, [rdx]
vmovapd ymm9, [rdx]
vmovapd ymm10, [rdx]
vmovapd ymm11, [rdx]
vmovapd ymm12, [rdx]
vmovapd ymm13, [rdx]
vmovapd ymm14, [rdx]
vmovapd ymm15, [rdx]

vzeroall

hlt

align 32
.data:
dq 0x4142434445464748
dq 0x5152535455565758
dq 0x6162636465666768
dq 0x7172737475767778
