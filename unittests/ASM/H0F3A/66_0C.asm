%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x4054c664c2f837b5", "0x40516053e2d6238e"],
    "XMM1":  ["0x4044836d7aa25d8d", "0x402a1e1c58255b03"],
    "XMM2":  ["0x4047ec6bc9d9d346", "0x4035fe425aee6320"],
    "XMM3":  ["0x4047ec6b7aa25d8d", "0x40154b7d41743e96"],
    "XMM4":  ["0x403d075a31a4bdba", "0x4050a01882d38477"],
    "XMM5":  ["0x40334ec17aa25d8d", "0x4056d74082d38477"],
    "XMM6":  ["0x4047ec6bc7cd898b", "0x40497b1382d38477"],
    "XMM7":  ["0x4047ec6b7aa25d8d", "0x4037f9ca82d38477"],
    "XMM8":  ["0x4056a929888f861a", "0x4055031766e43aa8"],
    "XMM9":  ["0x4058bc1f7aa25d8d", "0x40550317c91d14e4"],
    "XMM10": ["0x4047ec6ba10e0221", "0x4055031700bcbe62"],
    "XMM11": ["0x4047ec6b7aa25d8d", "0x405503170ed3d85a"],
    "XMM12": ["0x40419d2253111f0c", "0x4055031782d38477"],
    "XMM13": ["0x40177e287aa25d8d", "0x4055031782d38477"],
    "XMM14": ["0x4047ec6b9f16b11c", "0x4055031782d38477"],
    "XMM15": ["0x4047ec6b7aa25d8d", "0x4055031782d38477"]
  },
  "HostFeatures": ["SSE4.1"]
}
%endif

lea rdx, [rel .data]

movaps xmm0,  [rdx + 16 * 0]
movaps xmm1,  [rdx + 16 * 1]
movaps xmm2,  [rdx + 16 * 2]
movaps xmm3,  [rdx + 16 * 3]
movaps xmm4,  [rdx + 16 * 4]
movaps xmm5,  [rdx + 16 * 5]
movaps xmm6,  [rdx + 16 * 6]
movaps xmm7,  [rdx + 16 * 7]
movaps xmm8,  [rdx + 16 * 8]
movaps xmm9,  [rdx + 16 * 9]
movaps xmm10, [rdx + 16 * 10]
movaps xmm11, [rdx + 16 * 11]
movaps xmm12, [rdx + 16 * 12]
movaps xmm13, [rdx + 16 * 13]
movaps xmm14, [rdx + 16 * 14]
movaps xmm15, [rdx + 16 * 15]

blendps xmm0,  [rdx + 16 * 16], 0000b
blendps xmm1,  [rdx + 16 * 16], 0001b
blendps xmm2,  [rdx + 16 * 16], 0010b
blendps xmm3,  [rdx + 16 * 16], 0011b
blendps xmm4,  [rdx + 16 * 16], 0100b
blendps xmm5,  [rdx + 16 * 16], 0101b
blendps xmm6,  [rdx + 16 * 16], 0110b
blendps xmm7,  [rdx + 16 * 16], 0111b
blendps xmm8,  [rdx + 16 * 16], 1000b
blendps xmm9,  [rdx + 16 * 16], 1001b
blendps xmm10, [rdx + 16 * 16], 1010b
blendps xmm11, [rdx + 16 * 16], 1011b
blendps xmm12, [rdx + 16 * 16], 1100b
blendps xmm13, [rdx + 16 * 16], 1101b
blendps xmm14, [rdx + 16 * 16], 1110b
blendps xmm15, [rdx + 16 * 16], 1111b

hlt

align 16
; 512bytes of random data
.data:
dq 83.0999,69.50512,41.02678,13.05881,5.35242,21.9932,9.67383,5.32372,29.02872,66.50151,19.30764,91.3633,40.45086,50.96153,32.64489,23.97574,90.64316,24.22547,98.9394,91.21715,90.80143,99.48407,64.97245,74.39838,35.22761,25.35321,5.8732,90.19956,33.03133,52.02952,58.38554,10.17531,47.84703,84.04831,90.02965,65.81329,96.27991,6.64479,25.58971,95.00694,88.1929,37.16964,49.52602,10.27223,77.70605,20.21439,9.8056,41.29389,15.4071,57.54286,9.61117,55.54302,52.90745,4.88086,72.52882,3.0201,56.55091,71.22749,61.84736,88.74295,47.72641,24.17404,33.70564,96.71303
