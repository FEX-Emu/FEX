%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0", "0"],
    "XMM1": ["0x40a7e92935462e9e", "0"],
    "XMM2": ["0", "0x40a0712d6903205c"],
    "XMM3": ["0x408c728276ca7656", "0x408c728276ca7656"],
    "XMM4": ["0", "0"],
    "XMM5": ["0x40c0cd5f41a95ce2", "0"],
    "XMM6": ["0", "0x40b84aaf198a4022"],
    "XMM7": ["0x40abf229b504629d", "0x40abf229b504629d"],
    "XMM8": ["0", "0"],
    "XMM9": ["0x40c8384d475e602a", "0"],
    "XMM10": ["0", "0x40c8d105fa49a70e"],
    "XMM11": ["0x40c248e5ffd69239", "0x40c248e5ffd69239"],
    "XMM12": ["0", "0"],
    "XMM13": ["0x40beb622c0fe35c7", "0"],
    "XMM14": ["0", "0x40b74171bb41b9ba"],
    "XMM15": ["0x40ac8195a7735fbe", "0x40ac8195a7735fbe"]
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
dppd xmm0,  [rel .data + 16 * 16], 1111_0000b
dppd xmm1,  [rel .data + 16 * 16], 1111_0001b
dppd xmm2,  [rel .data + 16 * 16], 1111_0010b
dppd xmm3,  [rel .data + 16 * 16], 1111_0011b
dppd xmm4,  [rel .data + 16 * 16], 1111_0100b
dppd xmm5,  [rel .data + 16 * 16], 1111_0101b
dppd xmm6,  [rel .data + 16 * 16], 1111_0110b
dppd xmm7,  [rel .data + 16 * 16], 1111_0111b
dppd xmm8,  [rel .data + 16 * 16], 1111_1000b
dppd xmm9,  [rel .data + 16 * 16], 1111_1001b
dppd xmm10, [rel .data + 16 * 16], 1111_1010b
dppd xmm11, [rel .data + 16 * 16], 1111_1011b
dppd xmm12, [rel .data + 16 * 16], 1111_1100b
dppd xmm13, [rel .data + 16 * 16], 1111_1101b
dppd xmm14, [rel .data + 16 * 16], 1111_1110b
dppd xmm15, [rel .data + 16 * 16], 1111_1111b

hlt
align 16
; 512bytes of random data
.data:
dq 83.0999,69.50512,41.02678,13.05881,5.35242,21.9932,9.67383,5.32372,29.02872,66.50151,19.30764,91.3633,40.45086,50.96153,32.64489,23.97574,90.64316,24.22547,98.9394,91.21715,90.80143,99.48407,64.97245,74.39838,35.22761,25.35321,5.8732,90.19956,33.03133,52.02952,58.38554,10.17531,47.84703,84.04831,90.02965,65.81329,96.27991,6.64479,25.58971,95.00694,88.1929,37.16964,49.52602,10.27223,77.70605,20.21439,9.8056,41.29389,15.4071,57.54286,9.61117,55.54302,52.90745,4.88086,72.52882,3.0201,56.55091,71.22749,61.84736,88.74295,47.72641,24.17404,33.70564,96.71303
