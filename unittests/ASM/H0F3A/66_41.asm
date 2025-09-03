%ifdef CONFIG
{
  "RegData": {
    "XMM1": ["0x41278c496c911a6e", "0x41278c496c911a6e"],
    "XMM2": ["0x41235ccc64afb361", "0x41235ccc64afb361"],
    "XMM3": ["0x412bace273945dc5", "0x412bace273945dc5"],
    "XMM4": ["0x412cf22ef582fd76", "0x412cf22ef582fd76"],
    "XMM5": ["0x4121c80e40f3bc7b", "0x4121c80e40f3bc7b"],
    "XMM6": ["0", "0"],
    "XMM7": ["0", "0"],
    "XMM8": ["0", "0"]
  },
  "HostFeatures": ["SSE4.1"]
}
%endif

lea rdx, [rel .data]

movaps xmm1, [rdx + 16 * 0]
movaps xmm2, [rdx + 16 * 1]
movaps xmm3, [rdx + 16 * 2]
movaps xmm4, [rdx + 16 * 3]
movaps xmm5, [rdx + 16 * 4]
movaps xmm6, [rdx + 16 * 5]
movaps xmm7, [rdx + 16 * 6]
movaps xmm8, [rdx + 16 * 7]

dppd xmm1, [rdx + 16 * 8],  11111111b
dppd xmm2, [rdx + 16 * 9],  11111111b
dppd xmm3, [rdx + 16 * 10], 11111111b
dppd xmm4, [rdx + 16 * 11], 11111111b
dppd xmm5, [rdx + 16 * 12], 11111111b
dppd xmm6, [rdx + 16 * 13], 00000000b
dppd xmm7, [rdx + 16 * 14], 11110000b
dppd xmm8, [rdx + 16 * 15], 00001111b

hlt

align 16
; 256bytes of random data
.data:
dq 470.4127,683.87,711.3545,511.5631,996.8793,548.682,588.9345,832.5925,210.6613,792.6059,298.4494,154.4895,818.4,881.6027,705.3087,687.478,737.0665,621.31,755.3097,189.9614,552.4284,649.1206,798.252,574.5732,593.7565,577.3129,383.3844,443.3476,414.3571,615.1567,94.898,438.3107
