%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0xc2f837b5c2f837b5", "0x7aa25d8d7aa25d8d"],
    "XMM1":  ["0x4044836d86ec17ec", "0x4054c664c2f837b5"],
    "XMM2":  ["0x4035fe425aee6320", "0x4054c664c2f837b5"],
    "XMM3":  ["0x402359003eea209b", "0x4054c664c2f837b5"],
    "XMM4":  ["0x4050a018bd66277c", "0x402359003eea209b"],
    "XMM5":  ["0x4ea4a8c17ebaf102", "0x3eea209bbd66277c"],
    "XMM6":  ["0x40497b13404439b5", "0x3eea209b4ea4a8c1"],
    "XMM7":  ["0x4040528b4040528b", "0x3eea209b4ea4a8c1"],
    "XMM8":  ["0x403839b8403839b8", "0x3eea209b4ea4a8c1"],
    "XMM9":  ["0x4056cde54056cde5", "0x403839b8403839b8"],
    "XMM10": ["0x4056b34aa10e0221", "0x4056cde54056cde5"],
    "XMM11": ["0x4052997f0ed3d85a", "0xa10e0221a10e0221"],
    "XMM12": ["0x40419d2240395a6b", "0xa10e0221a10e0221"],
    "XMM13": ["0x40177e2840568cc5", "0x40419d2240395a6b"],
    "XMM14": ["0x9f16b11c40408402", "0x40419d2240395a6b"],
    "XMM15": ["0x5feda661404d3159", "0x9f16b11c40408402"]
  }
}
%endif

movaps xmm0, [rel .data + 16 * 0]
movaps xmm1, [rel .data + 16 * 1]

movaps xmm2, [rel .data + 16 * 2]
movaps xmm3, [rel .data + 16 * 3]

movaps xmm4, [rel .data + 16 * 4]
movaps xmm5, [rel .data + 16 * 5]

movaps xmm6, [rel .data + 16 * 6]
movaps xmm7, [rel .data + 16 * 7]

movaps xmm8, [rel .data + 16 * 8]
movaps xmm9, [rel .data + 16 * 9]

movaps xmm10, [rel .data + 16 * 10]
movaps xmm11, [rel .data + 16 * 11]

movaps xmm12, [rel .data + 16 * 12]
movaps xmm13, [rel .data + 16 * 13]

movaps xmm14, [rel .data + 16 * 14]
movaps xmm15, [rel .data + 16 * 15]

; Test inverted sources from shufps_optimization.asm
shufps xmm1, xmm0, 01000100b
shufps xmm0, [rel .data + 16 * 16], 0
shufps xmm2, xmm1, 11101110b
shufps xmm3, xmm2, 11100100b
shufps xmm4, xmm3, 01001110b
shufps xmm5, xmm4, 10001000b
shufps xmm6, xmm5, 11011101b
shufps xmm7, xmm6, 11100101b
shufps xmm8, xmm7, 11101111b
shufps xmm9, xmm8, 01001111b
shufps xmm10, xmm9, 00000100b
shufps xmm11, xmm10, 00001110b
shufps xmm12, xmm11, 11100111b
shufps xmm13, xmm12, 01000111b
shufps xmm14, xmm13, 11100001b
shufps xmm15, xmm14, 01000001b

hlt

align 16
; 512bytes of random data
.data:
dq 83.0999,69.50512,41.02678,13.05881,5.35242,21.9932,9.67383,5.32372,29.02872,66.50151,19.30764,91.3633,40.45086,50.96153,32.64489,23.97574,90.64316,24.22547,98.9394,91.21715,90.80143,99.48407,64.97245,74.39838,35.22761,25.35321,5.8732,90.19956,33.03133,52.02952,58.38554,10.17531,47.84703,84.04831,90.02965,65.81329,96.27991,6.64479,25.58971,95.00694,88.1929,37.16964,49.52602,10.27223,77.70605,20.21439,9.8056,41.29389,15.4071,57.54286,9.61117,55.54302,52.90745,4.88086,72.52882,3.0201,56.55091,71.22749,61.84736,88.74295,47.72641,24.17404,33.70564,96.71303
