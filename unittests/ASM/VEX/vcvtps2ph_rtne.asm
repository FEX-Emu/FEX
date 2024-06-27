%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0x008000003F800000", "0x7FC000007F7FFFFF", "0", "0"],
    "XMM1": ["0x7F80000040600000", "0x00000001FF800000", "0", "0"],
    "XMM2": ["0x7E007C0000003C00", "0x0000000000000000", "0", "0"],
    "XMM3": ["0x0000FC007C004300", "0x0000000000000000", "0", "0"],
    "XMM4": ["0x7E007C0000003C00", "0x0000FC007C004300", "0", "0"],
    "XMM5": ["0x008000003F800000", "0x7FC000007F7FFFFF", "0x7F80000040600000", "0x00000001FF800000"],
    "XMM6": ["0x7E007C0000003C00", "0x0000FC007C004300", "0", "0"],
    "XMM7": ["0x7E007C0000003C00", "0x0000FC007C004300", "0", "0"],
    "XMM8": ["0x4800440040003c00", "0xc800c400c000bc00", "0x4142434445464748", "0x4142434445464748"]
  }
}
%endif

; Round to Nearest Even

lea rdx, [rel .data]

; 128-bit

vmovapd xmm0, [rdx]
vmovapd xmm1, [rdx + 16]

; 128-bit register

vcvtps2ph xmm2, xmm0, 0
vcvtps2ph xmm3, xmm1, 0

; 128-bit memory
vcvtps2ph [rel .memarea + 0], xmm0, 0
vcvtps2ph [rel .memarea + 8], xmm1, 0
vmovapd xmm4, [rel .memarea]

; 256-bit

vmovapd ymm5, [rdx]

; 256-bit register

vcvtps2ph xmm6, ymm5, 0

; 256-bit memory

vcvtps2ph [rel .memarea + 16], ymm5, 0
vmovapd xmm7, [rel .memarea + 16]

; GCC test failure
vmovaps ymm8, [rel .data_in]
vcvtps2ph [rel .data_bad], ymm8, 0
vmovaps ymm8, [rel .data_bad]

hlt

align 32
.data:
dd 0x3F800000, 0x00800000, 0x7F7FFFFF, 0x7FC00000 ; 1.0, FLT_MIN, FLT_MAX, QNaN
dd 0x40600000, 0x7F800000, 0xFF800000, 0x00000001 ; 3.5, +inf, -inf, FLT_TRUE_MIN

; A quaint little area for testing the store variant of VCVTPS2PH
.memarea: times 16 dq 0

align 32
.data_in:
dd 1.0, 2.0, 4.0, 8.0, -1.0, -2.0, -4.0, -8.0

.data_bad:
dq 0x4142434445464748
dq 0x4142434445464748
dq 0x4142434445464748
dq 0x4142434445464748
