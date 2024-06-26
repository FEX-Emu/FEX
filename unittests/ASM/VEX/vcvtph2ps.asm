%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0xFC007C00BC003C00", "0x42487E0003FF0001", "0", "0"],
    "XMM1": ["0x800000007BFFFBFF", "0x42A53DC534D136F3", "0", "0"],
    "XMM2": ["0xBF8000003F800000", "0xFF8000007F800000", "0", "0"],
    "XMM3": ["0x477FE000C77FE000", "0x8000000000000000", "0", "0"],
    "XMM4": ["0xBF8000003F800000", "0xFF8000007F800000", "0", "0"],
    "XMM5": ["0x387FC00033800000", "0x404900007FC00000", "0", "0"],
    "XMM6": ["0xFC007C00BC003C00", "0x42487E0003FF0001", "0x800000007BFFFBFF", "0x42A53DC534D136F3"],
    "XMM7": ["0xBF8000003F800000", "0xFF8000007F800000", "0x387FC00033800000", "0x404900007FC00000"],
    "XMM8": ["0xBF8000003F800000", "0xFF8000007F800000", "0x387FC00033800000", "0x404900007FC00000"],
    "XMM9": ["0x477FE000C77FE000", "0x8000000000000000", "0x3E9A20003EDE6000", "0x4054A0003FB8A000"]
  }
}
%endif

lea rdx, [rel .data]

; 128-bit

vmovapd xmm0, [rdx]
vmovapd xmm1, [rdx + 16]

; 128-bit register
vcvtph2ps xmm2, xmm0
vcvtph2ps xmm3, xmm1

; 128-bit memory
vcvtph2ps xmm4, [rdx]
vcvtph2ps xmm5, [rdx + 8]

; 256-bit

vmovapd ymm6, [rdx]

; 256-bit register
vcvtph2ps ymm7, xmm6

; 256-bit memory
vcvtph2ps ymm8, [rdx]
vcvtph2ps ymm9, [rdx + 16]

hlt

align 32
.data:
dw 0x3C00 ; 1.0
dw 0xBC00 ; -1.0
dw 0x7C00 ; +inf
dw 0xFC00 ; -inf

dw 0x0001 ; min positive subnormal
dw 0x03FF ; max subnormal
dw 0x7E00 ; NaN
dw 0x4248 ; pi

dw 0xFBFF ; min finite value
dw 0x7BFF ; max finite value
dw 0x0000 ; +0.0
dw 0x8000 ; -0.0

dw 0x36F3 ; log_10(e)
dw 0x34D1 ; log_10(2)
dw 0x3DC5 ; log_2(e)
dw 0x42A5 ; log_2(10)
