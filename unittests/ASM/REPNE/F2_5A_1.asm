%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x4054c66442a63326", "0x40516053e2d6238e"],
    "XMM1": ["0x4044836d42241b6c", "0x402a1e1c58255b03"],
    "XMM2": ["0x401568e040ab4706", "0x4035fe425aee6320"],
    "XMM3": ["0x40235900411ac802", "0x40154b7d41743e96"],
    "XMM4": ["0x403d075a41e83ad2", "0x4050a018bd66277c"],
    "XMM5": ["0x40334ec1419a760c", "0x4056d7404ea4a8c1"],
    "XMM6": ["0x404439b54221cdae", "0x40497b136a400fbb"],
    "XMM7": ["0x4040528b4202945e", "0x4037f9ca18bd6627"],
    "XMM8": ["0x4056a92942b5494c", "0x403839b866e43aa8"],
    "XMM9": ["0x4058bc1f42c5e0f9", "0x4056cde5c91d14e4"],
    "XMM10": ["0x4056b34a42b59a55", "0x4058defb00bcbe62"],
    "XMM11": ["0x40503e3c4281f1e5", "0x4052997f0ed3d85a"],
    "XMM12": ["0x40419d22420ce913", "0x40395a6bf8769ec3"],
    "XMM13": ["0x40177e2840bbf141", "0x40568cc5974e65bf"],
    "XMM14": ["0x4040840242042015", "0x404a03c74fb549f9"],
    "XMM15": ["0x404d315942698acb", "0x402459c23b7952d2"]
  }
}
%endif

lea rdx, [rel .data]
movapd xmm0, [rdx + 16 * 0]
movapd xmm1, [rdx + 16 * 1]
movapd xmm2, [rdx + 16 * 2]
movapd xmm3, [rdx + 16 * 3]
movapd xmm4, [rdx + 16 * 4]
movapd xmm5, [rdx + 16 * 5]
movapd xmm6, [rdx + 16 * 6]
movapd xmm7, [rdx + 16 * 7]
movapd xmm8, [rdx + 16 * 8]
movapd xmm9, [rdx + 16 * 9]
movapd xmm10, [rdx + 16 * 10]
movapd xmm11, [rdx + 16 * 11]
movapd xmm12, [rdx + 16 * 12]
movapd xmm13, [rdx + 16 * 13]
movapd xmm14, [rdx + 16 * 14]
movapd xmm15, [rdx + 16 * 15]


cvtsd2ss xmm0, [rdx + 16 * 0]
cvtsd2ss xmm1, [rdx + 16 * 1]
cvtsd2ss xmm2, [rdx + 16 * 2]
cvtsd2ss xmm3, [rdx + 16 * 3]
cvtsd2ss xmm4, [rdx + 16 * 4]
cvtsd2ss xmm5, [rdx + 16 * 5]
cvtsd2ss xmm6, [rdx + 16 * 6]
cvtsd2ss xmm7, [rdx + 16 * 7]
cvtsd2ss xmm8, [rdx + 16 * 8]
cvtsd2ss xmm9, [rdx + 16 * 9]
cvtsd2ss xmm10, [rdx + 16 * 10]
cvtsd2ss xmm11, [rdx + 16 * 11]
cvtsd2ss xmm12, [rdx + 16 * 12]
cvtsd2ss xmm13, [rdx + 16 * 13]
cvtsd2ss xmm14, [rdx + 16 * 14]
cvtsd2ss xmm15, [rdx + 16 * 15]

hlt

align 16
; 512bytes of random data
.data:
dq 83.0999,69.50512,41.02678,13.05881,5.35242,21.9932,9.67383,5.32372,29.02872,66.50151,19.30764,91.3633,40.45086,50.96153,32.64489,23.97574,90.64316,24.22547,98.9394,91.21715,90.80143,99.48407,64.97245,74.39838,35.22761,25.35321,5.8732,90.19956,33.03133,52.02952,58.38554,10.17531,47.84703,84.04831,90.02965,65.81329,96.27991,6.64479,25.58971,95.00694,88.1929,37.16964,49.52602,10.27223,77.70605,20.21439,9.8056,41.29389,15.4071,57.54286,9.61117,55.54302,52.90745,4.88086,72.52882,3.0201,56.55091,71.22749,61.84736,88.74295,47.72641,24.17404,33.70564,96.71303
