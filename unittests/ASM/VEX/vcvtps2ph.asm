%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x4400420040003c00", "0"],
    "XMM1": ["0x4400420040003c00", "0"],
    "XMM2": ["0x4400420040003c00", "0"],
    "XMM3": ["0x4400420040003c00", "0"],
    "XMM4": ["0x4400420040003c00", "0"]
  }
}
%endif

movaps xmm5, [rel data1]
vcvtps2ph xmm0, xmm5, 4 ; host mode
vcvtps2ph xmm1, xmm5, 0 ; nearest
vcvtps2ph xmm2, xmm5, 1 ; down
vcvtps2ph xmm3, xmm5, 2 ; up
vcvtps2ph xmm4, xmm5, 3 ; truncate

hlt
align 16

data1:
dd 1.0
dd 2.0
dd 3.0
dd 4.0
