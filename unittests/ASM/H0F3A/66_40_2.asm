%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0", "0"],
    "XMM1": ["0x00000000c197f874", "0"],
    "XMM2": ["0xff80000000000000", "0x0000000000000000"],
    "XMM3": ["0x7a147e317a147e31", "0x0000000000000000"],
    "XMM4": ["0x0000000000000000", "0x000000006cd0f887"],
    "XMM5": ["0x000000007f800000", "0x000000007f800000"],
    "XMM6": ["0xff80000000000000", "0x00000000ff800000"],
    "XMM7": ["0xfc944256fc944256", "0x00000000fc944256"],
    "XMM8": ["0x0000000000000000", "0xc3ac072e00000000"],
    "XMM9": ["0x000000005c5c09a3", "0x5c5c09a300000000"],
    "XMM10": ["0xdc34227c00000000", "0xdc34227c00000000"],
    "XMM11": ["0xda1627d2da1627d2", "0xda1627d200000000"],
    "XMM12": ["0x0000000000000000", "0x7f8000007f800000"],
    "XMM13": ["0x000000005f30e9d3", "0x5f30e9d35f30e9d3"],
    "XMM14": ["0xda3f264a00000000", "0xda3f264ada3f264a"],
    "XMM15": ["0x7f8000007f800000", "0x7f8000007f800000"]
  },
  "HostFeatures": ["SSE4.1"]
}
%endif

movaps xmm0,  [rel .data + 16 * 0]
movaps xmm1,  [rel .data + 16 * 1]
movaps xmm2,  [rel .data + 16 * 2]
movaps xmm3,  [rel .data + 16 * 3]
movaps xmm4,  [rel .data + 16 * 4]
movaps xmm5,  [rel .data + 16 * 5]
movaps xmm6,  [rel .data + 16 * 6]
movaps xmm7,  [rel .data + 16 * 7]
movaps xmm8,  [rel .data + 16 * 8]
movaps xmm9,  [rel .data + 16 * 9]
movaps xmm10, [rel .data + 16 * 10]
movaps xmm11, [rel .data + 16 * 11]
movaps xmm12, [rel .data + 16 * 12]
movaps xmm13, [rel .data + 16 * 13]
movaps xmm14, [rel .data + 16 * 14]
movaps xmm15, [rel .data + 16 * 15]

; Full source mask but different broadcast tests
dpps xmm0,  [rel .data + 16 * 16], 1111_0000b
dpps xmm1,  [rel .data + 16 * 16], 1111_0001b
dpps xmm2,  [rel .data + 16 * 16], 1111_0010b
dpps xmm3,  [rel .data + 16 * 16], 1111_0011b
dpps xmm4,  [rel .data + 16 * 16], 1111_0100b
dpps xmm5,  [rel .data + 16 * 16], 1111_0101b
dpps xmm6,  [rel .data + 16 * 16], 1111_0110b
dpps xmm7,  [rel .data + 16 * 16], 1111_0111b
dpps xmm8,  [rel .data + 16 * 16], 1111_1000b
dpps xmm9,  [rel .data + 16 * 16], 1111_1001b
dpps xmm10, [rel .data + 16 * 16], 1111_1010b
dpps xmm11, [rel .data + 16 * 16], 1111_1011b
dpps xmm12, [rel .data + 16 * 16], 1111_1100b
dpps xmm13, [rel .data + 16 * 16], 1111_1101b
dpps xmm14, [rel .data + 16 * 16], 1111_1110b
dpps xmm15, [rel .data + 16 * 16], 1111_1111b

hlt
align 16
; 512bytes of random data
.data:
dq 83.0999,69.50512,41.02678,13.05881,5.35242,21.9932,9.67383,5.32372,29.02872,66.50151,19.30764,91.3633,40.45086,50.96153,32.64489,23.97574,90.64316,24.22547,98.9394,91.21715,90.80143,99.48407,64.97245,74.39838,35.22761,25.35321,5.8732,90.19956,33.03133,52.02952,58.38554,10.17531,47.84703,84.04831,90.02965,65.81329,96.27991,6.64479,25.58971,95.00694,88.1929,37.16964,49.52602,10.27223,77.70605,20.21439,9.8056,41.29389,15.4071,57.54286,9.61117,55.54302,52.90745,4.88086,72.52882,3.0201,56.55091,71.22749,61.84736,88.74295,47.72641,24.17404,33.70564,96.71303
